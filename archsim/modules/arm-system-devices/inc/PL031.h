/*
 * File:   PL031.h
 * Author: s0457958
 *
 * Created on 19 September 2014, 15:00
 */

#ifndef PL031_H
#define	PL031_H

#include "PrimecellRegisterDevice.h"

namespace archsim
{
	namespace abi
	{
		namespace external
		{
			namespace arm
			{
				class PL031 : public PrimecellRegisterDevice
				{
				public:
					PL031(EmulationModel& parent_model, Address base_address);
					~PL031();

					uint32_t ReadRegister(MemoryRegister& reg) override;
					void WriteRegister(MemoryRegister& reg, uint32_t value) override;
					
					bool Initialise() override;
				};
			}
		}
	}
}


#endif	/* PL031_H */

