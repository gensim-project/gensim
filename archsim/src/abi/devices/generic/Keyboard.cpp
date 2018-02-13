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
