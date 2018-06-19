/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PL080.h
 *
 *  Created on: 9 Jan 2014
 *      Author: harry
 */

#ifndef PL080_H_
#define PL080_H_

#include <stdint.h>
#include "abi/devices/Component.h"

/* ARM PL080 DMA Controller */

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class PL080 : public MemoryComponent
			{
			public:
				PL080(EmulationModel &parent, Address base_address);
				~PL080();

				bool Initialise() override;

				bool Read(uint32_t offset, uint8_t size, uint32_t& data);
				bool Write(uint32_t offset, uint8_t size, uint32_t data);
			};

		}
	}
}



#endif /* PL080_H_ */
