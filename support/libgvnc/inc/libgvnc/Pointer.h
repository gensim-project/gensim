/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include <stdint.h>
#include <functional>

namespace libgvnc
{
	struct PointerEvent {
		uint8_t ButtonMask;
		uint16_t PositionX;
		uint16_t PositionY;
	};

	using PointerCallback = std::function<void(struct PointerEvent&) >;

	class Pointer
	{
	public:

		void SendEvent(struct PointerEvent &event)
		{
			callback_(event);
		}

		void SetCallback(const PointerCallback &callback)
		{
			callback_ = callback;
		}

	private:
		PointerCallback callback_;
	};
}
