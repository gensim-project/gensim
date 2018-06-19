/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * GenericPrimecellDevice.h
 *
 *  Created on: 10 Jan 2014
 *      Author: harry
 */

#ifndef GENERICPRIMECELLDEVICE_H_
#define GENERICPRIMECELLDEVICE_H_

#include "abi/devices/Component.h"
#include <stdint.h>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class GenericPrimecellDevice : public MemoryComponent
			{
			public:
				GenericPrimecellDevice(EmulationModel &parent, Address base_address);
				~GenericPrimecellDevice();

				COMPONENT_PARAMETER_U64(PeripheralID);
				COMPONENT_PARAMETER_U64(PrimecellID);

				bool Read(uint32_t offset, uint8_t size, uint32_t& data);
				bool Write(uint32_t offset, uint8_t size, uint32_t data);

				bool Initialise() override;
			};

		}
	}
}

#endif /* GENERICPRIMECELLDEVICE_H_ */
