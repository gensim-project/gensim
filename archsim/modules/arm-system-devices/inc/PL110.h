/*
 * PL110.h
 *
 *  Created on: 21 Jul 2014
 *      Author: harry
 */

#ifndef PL110_H_
#define PL110_H_

#include "abi/devices/gfx/VirtualScreen.h"
#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class PL110 : public MemoryComponent
			{
			public:
				PL110(EmulationModel &parent, Address base_address);
				~PL110();

				bool Initialise() override;

				inline bool SetIrqLine(IRQLine *nirq)
				{
					irq = nirq;
					return true;
				}

				bool Read(uint32_t offset, uint8_t size, uint32_t& data) override;
				bool Write(uint32_t offset, uint8_t size, uint32_t data) override;

			private:
				void update_irq();
				void update_control();
				void update_palette();

				void enable_screen();
				void disable_screen();

				COMPONENT_PARAMETER_ENTRY(Screen, VirtualScreen, gfx::VirtualScreen);
				IRQLine *irq;

				uint32_t timing[4];

				uint32_t raw_palette[0x200 >> 2];

				uint32_t upper_fbbase, lower_fbbase;
				uint32_t control;
				uint32_t irq_mask, irq_raw;

				uint32_t internal_ppl;
				uint32_t internal_lines;
				bool internal_enabled;
			};

		}
	}
}

#endif /* PL110_H_ */
