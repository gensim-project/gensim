/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   PL061.h
 * Author: s0457958
 *
 * Created on 16 September 2014, 10:20
 */

#ifndef PL061_H
#define	PL061_H

#include "PrimecellRegisterDevice.h"
#include "abi/devices/generic/GPIO.h"
#include "abi/devices/IRQController.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class IRQLine;
		}

		namespace external
		{
			namespace arm
			{
				using namespace archsim::abi::devices;
				class PL061 : public PrimecellRegisterDevice
				{
				public:
					PL061(EmulationModel &parent, Address base_address);
					~PL061();

					bool Initialise() override;

					bool Read(uint32_t offset, uint8_t size, uint32_t& data) override;

					uint32_t ReadRegister(MemoryRegister& reg) override;
					void WriteRegister(MemoryRegister& reg, uint32_t value) override;

					generic::GPIO *AttachGPIO(unsigned int idx);

				private:
					bool HandleDataRead(uint32_t offset, uint8_t size, uint32_t& data);

					COMPONENT_PARAMETER_ENTRY(IRQLine, IRQLine, IRQLine);

					MemoryRegister GPIODir;
					MemoryRegister GPIOIS;
					MemoryRegister GPIOIBE;
					MemoryRegister GPIOIEV;
					MemoryRegister GPIOIE;
					MemoryRegister GPIORIS;
					MemoryRegister GPIOMIS;
					MemoryRegister GPIOIC;
					MemoryRegister GPIOAFSEL;
				};
			}
		}
	}
}

#endif	/* PL061_H */

