/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "abi/devices/generic/net/NetworkInterface.h"

#include <cassert>

using namespace archsim::abi::devices::generic::net;

void NetworkInterface::invoke_receive(const uint8_t* buffer, uint32_t length)
{
	if(_receive_callback) {
		_receive_callback(buffer, length);
	}
}