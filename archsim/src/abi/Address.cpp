#include "abi/Address.h"

#include <iostream>

using namespace archsim;

namespace archsim {

  std::ostream &operator<<(std::ostream &str, const archsim::Address &address)
  {
    str << "0x" << std::hex << std::setw(8) << std::setfill('0') << address.Get();
    return str;
  }

}
