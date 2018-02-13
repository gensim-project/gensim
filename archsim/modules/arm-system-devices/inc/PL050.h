/*
 * PL050.h
 *
 *  Created on: 28 Aug 2014
 *      Author: harry
 */

#ifndef PL050_H_
#define PL050_H_

#include "abi/devices/generic/ps2/PS2Device.h"
#include "PrimecellRegisterDevice.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				namespace ps2
				{
					class PS2Device;
				}
			}

		}
		namespace external
		{
			namespace arm
			{
				using namespace archsim::abi::devices;
				class PL050 : public PrimecellRegisterDevice
				{
				public:
					PL050(EmulationModel& parent, Address base_address);
					~PL050();

					bool Initialise() override;

				protected:
					uint32_t ReadRegister(MemoryRegister& reg) override;
					void WriteRegister(MemoryRegister& reg, uint32_t value) override;

				private:
					uint32_t last;

					COMPONENT_PARAMETER_ENTRY(PS2, PS2Device, devices::generic::ps2::PS2Device);

					MemoryRegister KMICR;
					MemoryRegister KMISTAT;
					MemoryRegister KMIDATA;
					MemoryRegister KMICLKDIV;
					MemoryRegister KMIIR;
				};
			}
		}
	}
}

#endif /* PL050_H_ */
