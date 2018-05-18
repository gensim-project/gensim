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
#include <libgvnc/net/SockAddr.h>
#include <malloc.h>
#include <cstring>

using namespace libgvnc::net;

SockAddrContainer::SockAddrContainer(size_t size) : data_(malloc(size)), size_(size)
{

}

SockAddrContainer::SockAddrContainer(const SockAddrContainer& other) : data_(malloc(other.size_)), size_(other.size_)
{
	::memcpy(data_, other.data_, size_);
}

SockAddrContainer::SockAddrContainer(SockAddrContainer&& other) : data_(other.data_), size_(other.size_)
{
	other.data_ = nullptr;
}

SockAddrContainer::~SockAddrContainer()
{
	::free(data_);
}
