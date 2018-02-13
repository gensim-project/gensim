/*
 * SP810.cpp
 *
 *  Created on: 16 Dec 2013
 *      Author: harry
 */
#include "SP810.h"
#include "abi/devices/generic/timing/TickSource.h"
#include "abi/EmulationModel.h"
#include "util/LogContext.h"
#include "system.h"

#include <chrono>
#include <stdio.h>
UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogSP810, LogArmExternalDevice, "SP810");

#define LOCK_VALUE 0xa05f

namespace archsim
{
	namespace abi
	{
		namespace devices
		{


			static ComponentDescriptor sp810_descriptor ("SP810");
			SP810::SP810(EmulationModel &parent, Address base_address) : MemoryComponent(parent, base_address, 0x1000), Component(sp810_descriptor), parent(parent), hr_begin(parent.GetSystem().GetTickSource()->GetCounter()), colour_mode(0x1f00), leds(0), lockval(0), flags(0)
			{

			}

			SP810::~SP810()
			{

			}

			bool SP810::Initialise()
			{
				return true;
			}

			bool SP810::Read(uint32_t offset, uint8_t size, uint32_t& data)
			{
//	fprintf(stderr, "SP810 Read\n");
				uint32_t reg = offset & 0xfff;

				LC_DEBUG2(LogSP810) << "Read from offset " << std::hex << offset;

				switch(reg) {
					case 0x000: //SYS_ID
//		data = 0x41008004;
						data = 0x01780500;
						break;
					case 0x004: //SYS_SW
						data = 0x0;
						break;
					case 0x008:
						data = leds;
						break;

//	case 0x1c:
//		data = 0x0;
//		break;
					case 0x20:
						data = lockval;
						break;

					case 0x24: { //100Hz Counter
						typedef std::chrono::duration<uint32_t, std::ratio<1, 100> > tick_100Hz_t;

						// default counter tick rate is 1ms
						uint64_t tick_count = parent.GetSystem().GetTickSource()->GetCounter() - hr_begin;
						std::chrono::milliseconds tick_time (tick_count);

						data = (std::chrono::duration_cast<tick_100Hz_t>(tick_time)).count();
						LC_DEBUG1(LogSP810) << "100Hz counter returned " << data;
						break;
					}
					case 0x30:
						data = flags;
						break;
					case 0x50: //LCD ID field
						data = colour_mode;
						break;
					case 0x5C: { //24MHz Counter
						typedef std::chrono::duration<uint32_t, std::ratio<1, 24000000> > tick_24MHz_t;

						// default counter tick rate is 1ms

						if(false && archsim::options::InstructionTick) {
							static FILE *timer_file = NULL;
							if(!timer_file) {
								timer_file = fopen("qemu.24mhz", "r");
								if(!timer_file) exit(1);
							}
							char *line = NULL;
							size_t len = 0;
							ssize_t chars = getline(&line, &len, timer_file);
							if(chars < 0) {
								fprintf(stderr, "sp810 out of ticks\n");
								exit(1);

							} else {
								data = strtol(line, NULL, 10);
								free(line);
							}
						} else  {
							uint64_t tick_count = parent.GetSystem().GetTickSource()->GetCounter() - hr_begin;
							std::chrono::milliseconds tick_time (tick_count);

							// If we're using an instruction ticker, scale to 500MHz
							if(archsim::options::InstructionTick) {
								data = (((uint64_t)tick_count) * 24000000) / 500000000;
							} else {
								data = (std::chrono::duration_cast<tick_24MHz_t>(tick_time)).count();
							}
						}

//		data = (std::chrono::duration_cast<tick_24MHz_t>(tick_time)).count();
						LC_DEBUG1(LogSP810) << "24MHz counter returned " << data;
						break;
					}
					case 0x84:
						data = 0x0c000000;
						return true;

					case 0x88:
						data = 0xff000000;
						return true;

					case 0x00c ... 0x001c:
//		data = osc[(offset - 0xc) >> 2];
						data = 0;
						break;
					case 0xfe0:
						data = 0x11;
						break;
					case 0xfe4:
						data = 0x10;
						break;
					case 0xfe8:
						data = 0x04;
						break;
					case 0xfec:
						data = 0x00;
						break;

					case 0xff0:
						data = 0x0D;
						break;
					case 0xff4:
						data = 0xF0;
						break;
					case 0xff8:
						data = 0x05;
						break;
					case 0xffc:
						data = 0xB1;
						break;

					default:
						LC_WARNING(LogSP810) << "Attempted to read a SP810 register which has not been implemented/does not exist " << std::hex << reg;
						data = 0;
						return false;
				}

				return true;

			}

			bool SP810::Write(uint32_t offset, uint8_t size, uint32_t data)
			{
//	fprintf(stderr, "SP810 Write\n");
				uint32_t reg = offset & 0xfff;
				switch(reg) {
					case 0x08:
						leds = data;
						return true;
					case 0x00c ... 0x01c:
						osc[(offset - 0x00c) >> 2] = data;
						return true;
					case 0x20:
						if(data == LOCK_VALUE) lockval = data;
						else lockval = data & 0x7fff;
						return true;
					case 0x30:
						flags |= data;
						return true;
					case 0x34:
						flags &= ~data;
						return true;
					case 0x50:
						colour_mode &= 0x3f00;
						colour_mode |= (data & ~0x3f00);
						LC_DEBUG1(LogSP810) << "Attempted to modify LCD register to " << std::hex << data;
						return true;
					default:
						LC_WARNING(LogSP810) << "[Versatile] Attempted to write a SP810 register which has not been implemented/does not exist " << std::hex << reg;
				}
				return false;
			}

		}
	}
}
