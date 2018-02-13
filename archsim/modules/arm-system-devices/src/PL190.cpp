/*
 * PL190.cpp
 *
 *  Created on: 16 Dec 2013
 *      Author: harry
 */

#include "PL190.h"

#include "abi/EmulationModel.h"
#include "util/LogContext.h"

#include <cassert>

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL190, LogArmExternalDevice, "PL190");

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			static ComponentDescriptor pl190con_desc ("PL190Controller");
			PL190::PL190Controller::PL190Controller(PL190 &parent) : IRQController(32), parent(parent), Component(pl190con_desc)
			{

			}
			
			bool PL190::PL190Controller::Initialise() {
				return true;
			}

			bool PL190::PL190Controller::AssertLine(uint32_t line_no)
			{
				assert(line_no < 32);

				LC_DEBUG1(LogPL190) << "Interrupt " << line_no << " asserted on PL190";

				parent.irq_level |= (1 << line_no);

				parent.update_lines();

				return true;
			}

			bool PL190::PL190Controller::RescindLine(uint32_t line_no)
			{
				assert(line_no < 32);

				LC_DEBUG1(LogPL190) << "Interrupt " << line_no << " rescinded on PL190";

				parent.irq_level &= ~(1 << line_no);

				parent.update_lines();

				return true;
			}

			void PL190::Dump()
			{
				fprintf(stderr, "PL190 IRQ mask: %08x Active IRQs: %08x\n", enable, get_active_irqs());
			}

			static ComponentDescriptor pl190_descriptor ("PL190", {{"IRQLine", ComponentParameter_Component}, {"FIQLine", ComponentParameter_Component}, {"IRQController", ComponentParameter_Component}});
			PL190::PL190(EmulationModel &parent, Address base_address) : MemoryComponent(parent, base_address, 0x10000), Component(pl190_descriptor), inner_controller(*this), default_vector_address(0)
			{
//	fiq_line->SetSource(&GetIRQController());
//	irq_line->SetSource(&GetIRQController());

				for(uint32_t i = 0; i < 16; ++i) {
					vector_addrs[i] = 0;
					vector_ctrls[i] = 0;
				}

				default_vector_address = 0;
				prio_mask[17] = 0xffffffff;
				priority = 17;

				enable = 0;
				soft_level = 0;
				irq_level = 0;
				fiq_select = 0;
				
				SetParameter<Component*>("IRQController", &inner_controller);
			}
			
			
			bool PL190::Initialise() {
				if(GetIRQLine() == nullptr || GetFIQLine() == nullptr) {
					return false;
				}
				
				update_vectors();
				
				return true;
			}

			PL190::~PL190()
			{

			}


			int PL190::GetComponentID()
			{
				return 0;
			}

			bool PL190::Read(uint32_t offset, uint8_t size, uint32_t& data)
			{
				uint32_t reg = offset & 0xfff;

				data = 0;

				if(reg < 0x100) {
					switch(reg) {
						case 0x000: // IRQ Status Register
							data = get_active_irqs();
							LC_DEBUG1(LogPL190) << "IRQStatus read";
							return true;
						case 0x004: // FIQ Status Register
							data = get_active_fiqs();
							return true;
						case 0x008: // Raw interrupt status register
							data = irq_level | soft_level;
							return true;
						case 0x00C: // Interrupt Select register
							data = fiq_select;
							return true;
						case 0x010: // Interrupt enable register
							data = enable;
							return true;
						case 0x014: // Interrupt enable clear register
							return false;
						case 0x018: // Software interrupt register
							data = soft_level;
							return true;
						case 0x01C: // Software interrupt clear register
							return false;
						case 0x020: // Protection enable register
							LC_WARNING(LogPL190) << "Attempted to access a protection PL190 register which has not been implemented " << std::hex << reg;
							return false;
						case 0x030: // Vector Address register
							data = handle_var_read();
							return true;
						case 0x034: // Default vector address register
							data = default_vector_address;
							return true;
						default:
							LC_WARNING(LogPL190) << "Attempted to access a PL190 register which does not exist " << std::hex << reg;
							return false;
					}
				} else if(reg < 0x200) {
					uint8_t vector = (reg - 0x200) / 4;
					if(vector > 16) return false;
					data = vector_addrs[vector];
					return true;
				} else if(reg < 0x23C) {
					uint8_t vector = (reg - 0x200) / 4;
					if(vector > 16) return false;

					data = vector_ctrls[vector];

					return true;
				} else if(reg >= 0xfe0) {
					switch(reg) {
						case 0xfe0:
							data = 0x90;
							break;
						case 0xfe4:
							data = 0x11;
							break;
						case 0xfe8:
							data = 0x04;
							break;
						case 0xfec:
							data = 0x00;
							break;
						case 0xff0:
							data = 0x0d;
							break;
						case 0xff4:
							data = 0xf0;
							break;
						case 0xff8:
							data = 0x05;
							break;
						case 0xffc:
							data = 0xb1;
							break;
					}

					return true;
				}

				LC_WARNING(LogPL190) << "Attempted to access unknown register " << std::hex << reg;
				return false;
			}

			bool PL190::Write(uint32_t offset, uint8_t size, uint32_t data)
			{
				uint32_t reg = offset & 0xfff;

				if(reg < 0x100) {
					switch(reg) {
						case 0x000: // IRQ Status Register
						case 0x004: // FIQ Status Register
						case 0x008: // Raw interrupt status register
							update_lines();
							return false;
						case 0x00C: // Interrupt Select register
							fiq_select = data;
							update_lines();
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to ISR";
							return true;
						case 0x010: // Interrupt enable register
							enable |= data;
							update_lines();
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to IER";
							return true;
						case 0x014: // Interrupt enable clear register
							enable &= ~data;
							update_lines();
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to IECR";
							return true;
						case 0x018: // Software interrupt register
							soft_level |= data;
							update_lines();
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to SIR";
							return true;
						case 0x01C: // Software interrupt clear register
							soft_level &= ~data;
							update_lines();
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to SICR";
							return true;
						case 0x020: // Protection enable register
							LC_WARNING(LogPL190) << "Attempted to access a protection enable PL190 register which has not been implemented " << std::hex << reg;
							return false;
						case 0x030: // Vector Address register
							handle_var_write();
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to VAR";
							return true;
						case 0x034: // Default vector address register
							default_vector_address = data;
							LC_DEBUG2(LogPL190) << "Write " << std::hex << data << " to DVAR";
							return true;
						default:
							LC_WARNING(LogPL190) << "Attempted to access a PL190 register which does not exist " << std::hex << reg;
							return false;
					}
				} else if(reg < 0x200) {
					uint8_t vector = (reg - 0x200) / 4;
					if(vector > 16) return false;
					vector_addrs[vector] = data;

					update_vectors();

					return true;
				} else if(reg < 0xFE0) {
					uint8_t vector = (reg - 0x200) / 4;
					if(vector > 16) return false;
					vector_ctrls[vector] = data;

					update_vectors();

					return true;
				}

				LC_WARNING(LogPL190) << "Attempted to access a PL190 register which does not exist " << std::hex << reg;
				return false;
			}

			void PL190::update_vectors()
			{
				uint32_t n, mask = 0;

				for(uint32_t i = 0; i < 16; ++i) {
					prio_mask[i] = mask;
					if(vector_ctrls[i] & 0x20) {
						n = vector_ctrls[i] & 0x1f;
						mask |= 1 << n;
					}
				}
				prio_mask[16] = mask;
				update_lines();
			}

			void PL190::update_lines()
			{
				//If there is an active IRQ of a higher priority than the current priority, assert the IRQ line
				if(get_active_irqs() & prio_mask[priority]) {
					//Raise the OUTGOING interrupt
					GetIRQLine()->Assert();
					LC_DEBUG1(LogPL190) << "IRQ raised " << std::hex << (get_active_irqs() & prio_mask[priority]);
				} else {
					//Clear the OUTGOING interrupt
					LC_DEBUG1(LogPL190) << "IRQ cleared: irqs: " << std::hex << get_active_irqs() << ", mask is " << prio_mask[priority];
					GetIRQLine()->Rescind();
				}

				//If there is an active FIQ of a higher priority than the current priority, assert the FIQ line
				if(get_active_fiqs() & prio_mask[priority]) GetFIQLine()->Assert();
				else GetFIQLine()->Rescind();
			}

			uint32_t PL190::handle_var_read()
			{
				uint32_t i = 0;
				for(i = 0; i < priority; ++i) {
					if((irq_level | soft_level) & prio_mask[i+1]) break;
				}
				if(i == 17) return default_vector_address;

				if(i < priority) {
					prev_priority[i] = priority;
					priority = i;
					LC_DEBUG1(LogPL190) << "Switching priority to " << i;
					update_lines();
				}
				return vector_addrs[priority];
			}

			void PL190::handle_var_write()
			{
				if(priority < 17) priority = prev_priority[priority];
				update_lines();
			}

		}
	}
}
