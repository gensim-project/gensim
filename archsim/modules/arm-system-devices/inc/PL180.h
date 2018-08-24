/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   PL180.h
 * Author: s0457958
 *
 * Created on 02 September 2014, 12:57
 */

#ifndef PL180_H
#define	PL180_H

#include "PrimecellRegisterDevice.h"
#include "abi/devices/IRQController.h"

namespace archsim
{
	namespace abi
	{

		namespace external
		{
			namespace arm
			{
				class PL180 : public PrimecellRegisterDevice
				{
				public:
					PL180(EmulationModel &parent, Address base_address);
					~PL180();

					bool Initialise() override;

					uint32_t ReadRegister(MemoryRegister& reg) override;
					void WriteRegister(MemoryRegister& reg, uint32_t value) override;

				private:
					void UpdateIRQs();

					COMPONENT_PARAMETER_ENTRY(mci0a_pri, IRQLine, IRQLine);
					COMPONENT_PARAMETER_ENTRY(mci0a_sec, IRQLine, IRQLine);
					COMPONENT_PARAMETER_ENTRY(mci0b_sec, IRQLine, IRQLine);

					MemoryRegister MCIPower;
					MemoryRegister MCIClock;
					MemoryRegister MCIArgument;
					MemoryRegister MMCCommand;
					MemoryRegister MCIRespCmd;
					MemoryRegister MCIResponse0;
					MemoryRegister MCIResponse1;
					MemoryRegister MCIResponse2;
					MemoryRegister MCIResponse3;
					MemoryRegister MCIDataTimer;
					MemoryRegister MCIDataLength;
					MemoryRegister MCIDataCtrl;
					MemoryRegister MCIDataCnt;
					MemoryRegister MCIStatus;
					MemoryRegister MCIClear;
					MemoryRegister MCIMask0;
					MemoryRegister MCIMask1;
					MemoryRegister MCISelect;
					MemoryRegister MCIFifoCnt;
					MemoryRegister MCIFIFO;
				};
			}
		}
	}
}

#endif	/* PL180_H */

