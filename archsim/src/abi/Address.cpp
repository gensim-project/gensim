/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "abi/Address.h"

#include <iostream>

using namespace archsim;

const archsim::Address archsim::Address::NullPtr (0);

namespace archsim
{

	std::ostream &operator<<(std::ostream &str, const archsim::Address &address)
	{
		str << "0x" << std::hex << std::setw(8) << std::setfill('0') << address.Get();
		return str;
	}

}
