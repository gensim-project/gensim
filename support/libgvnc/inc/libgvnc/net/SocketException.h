/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include <stdexcept>
#include <string>

namespace libgvnc {
    namespace net {
        class SocketException : public std::exception
        {
        public:
            SocketException(const std::string& message) : message_(message) { }
            
            const char* what() const noexcept override { return message_.c_str(); }
            
        private:
            const std::string message_;
        };
    }
}
