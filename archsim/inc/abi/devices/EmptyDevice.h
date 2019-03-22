/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"
#include "abi/devices/SerialPort.h"

#include <queue>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class EmptyDevice : public MemoryComponent
			{
			public:
				EmptyDevice(EmulationModel &parent, Address base_address, uint64_t size);
				virtual ~EmptyDevice();

				bool Initialise() override;
				bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
				bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

			};
		}
	}
}
