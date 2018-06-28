/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/net/NetworkInterface.h"

#include <cassert>

using namespace archsim::abi::devices::generic::net;

void NetworkInterface::invoke_receive(const uint8_t* buffer, uint32_t length)
{
	if(_receive_callback) {
		_receive_callback(buffer, length);
	}
}
