/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/Address.h"

#include <iostream>

using namespace archsim;

const archsim::Address archsim::Address::NullPtr (0);

namespace archsim
{

	std::ostream &operator<<(std::ostream &str, const archsim::Address &address)
	{
		auto prevflags = str.flags();
		str << "0x" << std::hex << std::setw(8) << std::setfill('0') << address.Get();
		str.flags(prevflags);
		return str;
	}

}

archsim::Address _get_address(Address::underlying_t addr)
{
	return Address(addr);
}