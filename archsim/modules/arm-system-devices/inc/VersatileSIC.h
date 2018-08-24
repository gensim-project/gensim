/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * VersatileSIC.h
 *
 *  Created on: 16 Dec 2013
 *      Author: harry
 */

#ifndef VERSATILESIC_H_
#define VERSATILESIC_H_

#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			template<int nr_lines = 32>
			class VersatileSIC : public RegisterBackedMemoryComponent, public IRQController
			{
			public:
				VersatileSIC(EmulationModel &parent, Address base_address);
				virtual ~VersatileSIC();

				bool Initialise() override;

				COMPONENT_PARAMETER_ENTRY(IRQLine, IRQLine, IRQLine);

				bool AssertLine(uint32_t line) override;
				bool RescindLine(uint32_t line) override;

				uint32_t ReadRegister(MemoryRegister& reg) override;
				void WriteRegister(MemoryRegister& reg, uint32_t value) override;

			private:
				MemoryRegister SIC_STATUS;
				MemoryRegister SIC_RAWSTAT;
				MemoryRegister SIC_ENABLE;
				MemoryRegister SIC_ENCLR;
				MemoryRegister SIC_SOFTINTSET;
				MemoryRegister SIC_SOFTINTCLR;
				MemoryRegister SIC_PICENABLE;
				MemoryRegister SIC_PICENCLR;

				void UpdateParentState();

				bool lines[nr_lines];
			};
		}
	}
}

#endif /* VERSATILESIC_H_ */
