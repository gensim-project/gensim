/*
 * PL080.cpp
 *
 *  Created on: 9 Jan 2014
 *      Author: harry
 */


/* PL080 DMA Controller */


#include "PL080.h"
#include "util/LogContext.h"

#include <iomanip>

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL080, LogArmExternalDevice, "PL080");

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			static ComponentDescriptor pl080_descriptor ("PL080");
			PL080::PL080(EmulationModel &parent, Address base_addr) :  MemoryComponent(parent, base_addr, 0x10000), Component(pl080_descriptor)
			{

			}

			PL080::~PL080()
			{

			}
			
			bool PL080::Initialise()
			{
				return true;
			}

			bool PL080::Read(uint32_t offset, uint8_t size, uint32_t& data)
			{
				uint32_t reg = (offset & 0xffff);
				switch(reg) {
					case 0xfe0:
						data = 0x80;
						break;
					case 0xfe4:
						data = 0x10;
						break;
					case 0xfe8:
						data = 0x04;
						break;
					case 0xfec:
						data = 0x0A;
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
						LC_WARNING(LogPL080) << "Attempted to access unimplemented/non-existant register " << std::hex << reg;
						break;
				}
				return false;
			}

			bool PL080::Write(uint32_t offset, uint8_t size, uint32_t data)
			{
				uint32_t reg = (offset & 0xffff);
				switch(reg) {
					default:
						LC_WARNING(LogPL080) << "Attempted to access unimplemented/non-existant register " << std::hex << reg;
						break;
				}
				return false;
			}

		}
	}
}
