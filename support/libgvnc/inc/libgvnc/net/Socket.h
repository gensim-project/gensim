#pragma once

#include "Types.h"

#include <cstddef>

namespace libgvnc {
    namespace net {
        class EndPoint;
        
        class Socket
        {
        public:
            Socket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType);
            ~Socket();
            
            void Listen(int backlog);
            void Bind(const EndPoint& endpoint);
            void Close();
            
            Socket *Accept();
            
            int Read(void *buffer, size_t size);
            int Write(const void *buffer, size_t size);
            
        private:
            Socket(int native_fd, const EndPoint& remoteEndPoint);
            
            void EnsureNotClosed();
            
            AddressFamily addressFamily_;
            SocketType socketType_;
            ProtocolType protocolType_;
            
            int fd_;
        };
    }
}
