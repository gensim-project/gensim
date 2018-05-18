/*
 * Copyright (C) 2018 Harry Wagstaff, Tom Spink
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
