#include "abi/devices/generic/Mouse.h"

using namespace archsim::abi::devices::generic;

void MouseAggregator::ButtonDown(int button_index)
{
	for (auto mouse : mice) {
		mouse->ButtonDown(button_index);
	}
}

void MouseAggregator::ButtonUp(int button_index)
{
	for (auto mouse : mice) {
		mouse->ButtonUp(button_index);
	}
}

void MouseAggregator::Move(int x, int y)
{
	for (auto mouse : mice) {
		mouse->Move(x, y);
	}
}
