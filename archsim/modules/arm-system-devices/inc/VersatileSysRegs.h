/*
 * VeratileSysRegs.h
 *
 *  Created on: 17 Dec 2013
 *      Author: harry
 */

#ifndef VERATILESYSREGS_H_
#define VERATILESYSREGS_H_


#include "abi/devices/Component.h"

#include <chrono>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class VersatileSysRegs : public MemoryComponent
			{
			public:
				VersatileSysRegs(EmulationModel &parent, uint32_t base_address);
				~VersatileSysRegs();

				int GetComponentID();

				bool Read(uint32_t offset, uint8_t size, uint32_t& data);
				bool Write(uint32_t offset, uint8_t size, uint32_t data);

				std::chrono::high_resolution_clock::time_point hr_begin;
			};

		}
	}
}



#endif /* VERATILESYSREGS_H_ */
