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
