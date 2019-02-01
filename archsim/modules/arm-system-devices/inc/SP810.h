/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * SP810.h
 *
 *  Created on: 16 Dec 2013
 *      Author: harry
 */

#ifndef SP810_H_
#define SP810_H_

#include "abi/devices/Component.h"

#include <chrono>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class SP810 : public MemoryComponent
			{
			public:
				SP810(EmulationModel &parent, Address base_address);
				~SP810();

				bool Initialise() override;

				bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
				bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

				uint64_t hr_begin;

			private:
				EmulationModel &parent;

				uint32_t osc[5];
				uint32_t colour_mode;
				uint32_t lockval, leds, flags;
			};

		}
	}
}

#endif /* SP810_H_ */
