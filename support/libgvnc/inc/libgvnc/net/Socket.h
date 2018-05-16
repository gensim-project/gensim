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
