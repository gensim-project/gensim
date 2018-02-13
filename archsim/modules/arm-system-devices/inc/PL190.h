/*
 * PL190.h
 *
 *  Created on: 16 Dec 2013
 *      Author: harry
 */

#ifndef PL190_H_
#define PL190_H_


#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"

#include <atomic>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class PL190 : public MemoryComponent
			{
				class PL190Controller : public IRQController
				{
				public:
					PL190Controller(PL190 &parent);

					PL190 &parent;

					virtual bool AssertLine(uint32_t line_no) override;

					virtual bool RescindLine(uint32_t line_no) override;
					
					bool Initialise() override;
				};

				friend class PL190Controller;

			public:

				PL190(EmulationModel &parent, Address base_address);
				~PL190();

				int GetComponentID();

				bool Read(uint32_t offset, uint8_t size, uint32_t& data);
				bool Write(uint32_t offset, uint8_t size, uint32_t data);

				inline uint32_t get_active_irqs() const
				{
					return (irq_level | soft_level) & enable & ~fiq_select;
				}
				inline uint32_t get_active_fiqs() const
				{
					return (irq_level | soft_level) & enable & fiq_select;
				}

				void Dump();

				COMPONENT_PARAMETER_ENTRY(IRQLine, IRQLine, IRQLine);
				COMPONENT_PARAMETER_ENTRY(FIQLine, IRQLine, IRQLine);
				COMPONENT_PARAMETER_ENTRY(IRQController, IRQController, IRQController);
				
				bool Initialise() override;

			private:
				std::atomic<uint32_t> irq_level;
				uint32_t soft_level, enable, fiq_select;

				uint32_t priority, prev_priority[17], prio_mask[18];

				uint8_t vector_ctrls[16];
				uint32_t vector_addrs[16];
				uint32_t default_vector_address;

				PL190Controller inner_controller;

				void update_lines();
				void update_vectors();

				uint32_t handle_var_read();
				void handle_var_write();
			};
		}
	}
}


#endif /* PL190_H_ */
