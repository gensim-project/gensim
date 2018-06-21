/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   PrimecellRegisterDevice.h
 * Author: s0457958
 *
 * Created on 02 September 2014, 11:27
 */

#ifndef PRIMECELLREGISTERDEVICE_H
#define	PRIMECELLREGISTERDEVICE_H

#include "abi/devices/Component.h"

namespace archsim
{
	namespace abi
	{
		namespace external
		{
			namespace arm
			{
				using namespace archsim::abi::devices;

				class PrimecellRegisterDevice : public RegisterBackedMemoryComponent
				{
				public:
					PrimecellRegisterDevice(EmulationModel& parent_model, Address base_address, uint32_t size, uint32_t periph_id, uint32_t primecell_id, std::string name);

				protected:
					uint32_t ReadRegister(MemoryRegister& reg) override;
					void WriteRegister(MemoryRegister& reg, uint32_t value) override;

				private:
					MemoryRegister PeriphID0;
					MemoryRegister PeriphID1;
					MemoryRegister PeriphID2;
					MemoryRegister PeriphID3;

					MemoryRegister PrimecellID0;
					MemoryRegister PrimecellID1;
					MemoryRegister PrimecellID2;
					MemoryRegister PrimecellID3;
				};
			}
		}
	}
}

#endif	/* PRIMECELLREGISTERDEVICE_H */

