/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <libgvnc/net/EndPoint.h>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace libgvnc::net;

IPAddress IPAddress::Any(0);
IPAddress IPAddress::Broadcast(0xffffffff);
IPAddress IPAddress::Loopback(0x7f000001);

SockAddrContainer IPEndPoint::GetSockAddr() const
{
	auto ctr = SockAddrContainer(sizeof(struct sockaddr_in));
	
	struct sockaddr_in *sin = (struct sockaddr_in *)ctr.GetSockAddr();
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port_);
	sin->sin_addr.s_addr = htonl(address_.GetAddress());
	
	return ctr;
}
