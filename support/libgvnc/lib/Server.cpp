/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libgvnc/Server.h"
#include "libgvnc/ClientConnection.h"
#include "libgvnc/net/EndPoint.h"

#include <stdexcept>

using namespace libgvnc;
using namespace libgvnc::net;

Server::Server(Framebuffer *fb) : state_(State::Closed), fb_(fb)
{

}

Server::~Server()
{

}

void Server::AddClientConnection(ClientConnection *client)
{
	clients_.insert(client);
	client->Open();
}

bool Server::Open(int listen_port)
{
	std::unique_lock<std::mutex> lg(lock_);

	if (state_ == State::Open) {
		throw std::logic_error("Server is already running");
	}

	listen_socket_ = new net::Socket(AddressFamily::INET, SocketType::STREAM, ProtocolType::NONE);

	IPEndPoint listenEndPoint(IPAddress::Any, listen_port);
	listen_socket_->Bind(listenEndPoint);
	listen_socket_->Listen(5);

	//int pingpong = 1;
	//setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &pingpong, sizeof(pingpong));

	// start a new thread to listen for and service connections
	state_ = State::Starting;
	server_thread_ = std::thread(server_thread_tramp, this);

	while (state_ != State::Open) {
		ready_cv_.wait(lg);
	}

	return true;
}

void *Server::ServerThread()
{
	NotifyServerReady();

	while (state_ == State::Open) {
		Socket *client_socket = listen_socket_->Accept();

		if (client_socket != nullptr) {
			AddClientConnection(new ClientConnection(this, client_socket));
		}
	}

	return nullptr;
}

void Server::NotifyServerReady()
{
	std::lock_guard<std::mutex> lg(lock_);
	state_ = State::Open;
	ready_cv_.notify_all();
}
