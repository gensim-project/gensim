/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "libgvnc/Server.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdexcept>

using namespace libgvnc;

void Server::server_thread(Server *server) {
	server->state_ = State::Open;
	
	listen(server->listen_socket_, 5);
	
	while(server->state_ == State::Open) {
		int sock_fd = accept(server->listen_socket_, nullptr, nullptr);
		server->AddClient(new Client(server, sock_fd));
	}
}

Server::Server(Framebuffer *fb) : state_(State::Closed), fb_(fb)
{
	
}

void Server::AddClient(Client* client)
{
	clients_.insert(client);
	client->Open();
}


bool Server::Open(int listen_port)
{
	std::lock_guard<std::mutex> lg(lock_);
	
	if(state_ == State::Open) {
		throw std::logic_error("Server is already running");
	}
	
	listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sa;
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(listen_port);
	
	int pingpong = 1;
	setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &pingpong, sizeof(pingpong));
	
	if(bind(listen_socket_, (sockaddr*)&sa, sizeof(sa)) < 0) {
		throw std::logic_error(std::string("Could not bind socket: ") + std::string(strerror(errno)));
	}
	
	// start a new thread to listen for and service connections
	server_thread_ = std::thread([this](){server_thread(this);});
	
	state_ = State::Open;
	
	return true;
}
