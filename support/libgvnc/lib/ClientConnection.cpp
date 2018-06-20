/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libgvnc/ClientConnection.h"
#include "libgvnc/Framebuffer.h"
#include "libgvnc/Server.h"

#include <arpa/inet.h>

using namespace libgvnc;
using namespace libgvnc::net;

ClientConnection::ClientConnection(Server *server, Socket *client_socket) : client_socket_(client_socket), state_(State::Invalid), server_(server)
{
}

void ClientConnection::Open()
{
	open_ = true;
	state_ = State::FreshlyConnected;
	thread_ = std::thread([this]() {
		try {
			Run();
		} catch(std::exception &e) {
			delete this;
		}
	});
}

void ClientConnection::Close()
{
	state_ = State::Closed;
	client_socket_->Close();
	delete client_socket_;
}

void ClientConnection::Run()
{
	if(!Handshake() || !Initialise()) {
		Close();
		return;
	}

	while(open_) {
		std::lock_guard<std::mutex> lg(lock_);

		ServeClientMessage();
	}
}

struct packet_protocolversion {
	char data[12];
};

void ClientConnection::SendRaw(const void* data, size_t size)
{
	if(client_socket_->Write(data, size) != size) {
		throw std::logic_error("Something went wrong: " + std::string(strerror(errno)));
	}
}

void ClientConnection::ReceiveRaw(void* data, size_t size)
{
	int total_bytes = 0;
	char *cdata = (char*)data;
	while(total_bytes < size) {
		int bytes = client_socket_->Read(cdata + total_bytes, size - total_bytes);
		if(bytes < 0) {
			throw std::logic_error("Something went wrong: " + std::string(strerror(errno)));
		}
		total_bytes += bytes;
	}
}


bool ClientConnection::Handshake()
{
	// send a protocolversion packet
	const char * const protocol_version = "RFB 003.008\n";
	struct packet_protocolversion version_packet;
	memcpy(version_packet.data, protocol_version, 12);

	SendRaw((void*)&version_packet, sizeof(version_packet));

	// wait to get one back
	ReceiveRaw((void *)&version_packet, sizeof(version_packet));

	// inspect the relevant part of the protocol version string to figure out
	// exactly the version we should use
	subversion_ = atoi(&version_packet.data[8]);

	// send security types
	uint8_t security_count = 1;
	Buffer(security_count);

	// (only support no security for now)
	uint8_t security_type = 1;
	Buffer(security_type);

	SendBuffer();

	// wait for a selection
	Receive(security_type);
	if(security_type != 1) {
		// unsupported
		uint32_t security_failed = 1;
		Send(security_failed);

		std::string reason = "No supported security mode";
		Send((uint32_t)reason.size());
		SendRaw((void*)reason.c_str(), reason.size());
		return false;
	}

	// report security result
	uint32_t security_ok = 0;
	Send(security_ok);

	// handshake complete
	state_ = State::HS_Done;

	return true;
}

struct serverinit_message {
	uint16_t fb_width;
	uint16_t fb_height;
	struct FB_PixelFormat pixelformat;
};

bool ClientConnection::Initialise()
{
	// wait for ClientInit message
	uint8_t clientinit = 0;
	Receive(clientinit);

	// don't actually care about the result just now
	struct serverinit_message serverinit;

	serverinit.fb_height = GetServer()->GetFB()->GetHeight();
	serverinit.fb_width = GetServer()->GetFB()->GetWidth();

	libgvnc::FB_PixelFormat Format_RGB32 (32, 24, 0, 1, 255, 255, 255, 16, 8, 0);
	serverinit.pixelformat = Format_RGB32;

	SetPixelFormat(serverinit.pixelformat);

	Buffer(htons(serverinit.fb_width));
	Buffer(htons(serverinit.fb_height));
	Buffer(serverinit.pixelformat.bits_per_pixel);
	Buffer(serverinit.pixelformat.depth);
	Buffer(serverinit.pixelformat.big_endian);
	Buffer(serverinit.pixelformat.true_color);
	Buffer(htons(serverinit.pixelformat.red_max));
	Buffer(htons(serverinit.pixelformat.green_max));
	Buffer(htons(serverinit.pixelformat.blue_max));
	Buffer(serverinit.pixelformat.red_shift);
	Buffer(serverinit.pixelformat.green_shift);
	Buffer(serverinit.pixelformat.blue_shift);
	Buffer(serverinit.pixelformat.padding);

	std::string name = GetServer()->GetFB()->GetTitle();
	Buffer((uint32_t)name.size());
	Buffer(name.c_str(), name.size());

	SendBuffer();

	state_ = State::Init_Done;

	return true;
}

bool ClientConnection::ServeClientMessage()
{
	uint8_t message_type;
	Receive(message_type);

	switch(message_type) {
		case 0:
			return ServeSetPixelFormat();
		case 2:
			return ServeSetEncodings();
		case 3:
			return ServeFramebufferUpdateRequest();
		case 4:
			return ServeKeyEvent();
		case 5:
			return ServePointerEvent();
		case 6:
			return ServeClientCutText();
		default:
			throw std::logic_error("Unknown message type");
	}
}

bool ClientConnection::ServeSetPixelFormat()
{
	uint8_t padding[3];
	ReceiveRaw(padding, 3);

	struct FB_PixelFormat format;
	Receive(format.bits_per_pixel);
	Receive(format.depth);
	Receive(format.big_endian);
	Receive(format.true_color);
	Receive(format.red_max);
	format.red_max = ntohs(format.red_max);

	Receive(format.green_max);
	format.green_max = ntohs(format.green_max);

	Receive(format.blue_max);
	format.blue_max = ntohs(format.blue_max);

	Receive(format.red_shift);
	Receive(format.green_shift);
	Receive(format.blue_shift);

	Receive(format.padding);

	SetPixelFormat(format);

	return true;
}

bool ClientConnection::ServeSetEncodings()
{
	uint8_t padding;
	Receive(padding);

	uint16_t number_encodings;
	Receive(number_encodings);
	number_encodings = ntohs(number_encodings);

	std::vector<int32_t> encodings;
	encodings.resize(number_encodings);
	ReceiveRaw(encodings.data(), sizeof(int32_t) * number_encodings);

	return true;
}

bool ClientConnection::ServeFramebufferUpdateRequest()
{
	struct fb_update_request request;
	Receive(request.incremental);
	Receive(request.x_pos);
	request.x_pos = ntohs(request.x_pos);

	Receive(request.y_pos);
	request.y_pos = ntohs(request.y_pos);

	Receive(request.width);
	request.width = ntohs(request.width);
	Receive(request.height);
	request.height = ntohs(request.height);

	UpdateFB(request);

	return true;
}

void ClientConnection::UpdateFB(const struct fb_update_request& request)
{
	// Message type
	Buffer((uint8_t)0);

	// Padding
	Buffer((uint8_t)0);

	auto result = GetServer()->GetFB()->ServeRequest(request, pixel_format_);
	Buffer((uint16_t)htons(result.size()));
	// result is a vector of rectangles
	for(auto &rectangle : result) {
		Buffer(htons(rectangle.Shape.X));
		Buffer(htons(rectangle.Shape.Y));
		Buffer(htons(rectangle.Shape.Width));
		Buffer(htons(rectangle.Shape.Height));
		Buffer(htonl((int32_t)rectangle.Encoding));
		Buffer(rectangle.Data.data(), rectangle.Data.size());
	}

	SendBuffer();
}

bool ClientConnection::ServeKeyEvent()
{
	uint16_t padding;
	struct KeyEvent event;
	Receive(event.Down);
	Receive(padding);
	Receive(event.KeySym);
	event.KeySym = ntohl(event.KeySym);

	GetServer()->GetKeyboard().SendEvent(event);

	return true;
}

bool ClientConnection::ServePointerEvent()
{
	struct PointerEvent event;
	Receive(event.ButtonMask);
	Receive(event.PositionX);
	event.PositionX = ntohs(event.PositionX);
	Receive(event.PositionY);
	event.PositionY = ntohs(event.PositionY);

	GetServer()->GetPointer().SendEvent(event);

	return true;
}

bool ClientConnection::ServeClientCutText()
{
	uint8_t padding[3];
	ReceiveRaw(padding, 3);

	uint32_t text_length;
	Receive(text_length);

	std::vector<char> chars;
	chars.resize(text_length);
	ReceiveRaw((void*)chars.data(), text_length);

	return true;
}
