/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include <cstddef>

struct sockaddr;

namespace libgvnc {
    namespace net {
        class SockAddrContainer {
        public:
            SockAddrContainer(const SockAddrContainer& other);
            SockAddrContainer(SockAddrContainer&& other);
            SockAddrContainer(size_t size);
            ~SockAddrContainer();
            
            const struct sockaddr *GetSockAddr() const { return (const struct sockaddr *)data_; }
            struct sockaddr *GetSockAddr() { return (struct sockaddr *)data_; }
            
            size_t GetSize() const { return size_; }
            
        private:
            void *data_;
            size_t size_;
        };
    }
}
