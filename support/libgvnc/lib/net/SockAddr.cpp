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
