/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/Keyboard.h"

using namespace archsim::abi::devices::generic;

void KeyboardAggregator::KeyUp(uint32_t keycode)
{
	for(auto kbd : keyboards) {
		kbd->KeyUp(keycode);
	}
}

void KeyboardAggregator::KeyDown(uint32_t keycode)
{
	for(auto kbd : keyboards) {
		kbd->KeyDown(keycode);
	}
}
