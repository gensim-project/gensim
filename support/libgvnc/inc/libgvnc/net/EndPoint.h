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
#include "SockAddr.h"

namespace libgvnc {
    namespace net {
        class EndPoint
        {
        public:
            virtual AddressFamily GetAddressFamily() const = 0;
            virtual SockAddrContainer GetSockAddr() const = 0;
        };
        
        class IPAddress
        {
        public:
            static IPAddress Any;
            static IPAddress Broadcast;
            static IPAddress Loopback;
            
            IPAddress(unsigned int address) : address_(address) { }
            
            unsigned int GetAddress() const { return address_; }
            
        private:
            unsigned int address_;
        };
        
        class IPEndPoint : public EndPoint
        {
        public:
            IPEndPoint(const IPAddress& address, int port) : address_(address), port_(port) { }
            
            AddressFamily GetAddressFamily() const { return AddressFamily::INET; }
            
            const IPAddress& GetAddress() const { return address_; }
            int GetPort() const { return port_; }
            
            SockAddrContainer GetSockAddr() const override;
            
        private:
            const IPAddress address_;
            int port_;
        };
    }
}
