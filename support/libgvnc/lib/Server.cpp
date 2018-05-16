/*
 * Copyright (C) 2018 Harry Wagstaff
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
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
