/*
 * GenericPrimecellDevice.cpp
 *
 *  Created on: 9 Jan 2014
 *      Author: harry
 */

#include "GenericPrimecellDevice.h"

#include "util/LogContext.h"

#include <iomanip>

UseLogContext(LogArmDevice);
DeclareChildLogContext(LogArmExternalDevice, LogArmDevice, "External");
DeclareChildLogContext(LogGenericPrimecell, LogArmExternalDevice, "Generic-Primecell");

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			static ComponentDescriptor generic_primecell_descriptor("GenericPrimecell", {{"PeripheralID", ComponentParameter_U64}, {"PrimecellID", ComponentParameter_U64}});
			GenericPrimecellDevice::GenericPrimecellDevice(EmulationModel &parent, Address base_addr) :  MemoryComponent(parent, base_addr, 0x1000), Component(generic_primecell_descriptor)
			{
				SetParameter("PrimecellID", (uint64_t)0x0df005b1);
				SetParameter("PeripheralID", (uint64_t)0x00000000);
			}

			GenericPrimecellDevice::~GenericPrimecellDevice()
			{

			}

			bool GenericPrimecellDevice::Initialise() {
				return true;
			}

			bool GenericPrimecellDevice::Read(uint32_t offset, uint8_t size, uint32_t& data)
			{
				uint32_t reg = (offset & (GetSize()-1));

				uint32_t peripheral_id = GetParameter<uint64_t>("PeripheralID");
				uint32_t primecell_id = GetParameter<uint64_t>("PrimecellID");

				switch(reg) {
					case 0xfe0:
						data = peripheral_id >> 24;
						break;
					case 0xfe4:
						data = peripheral_id >> 16;
						break;
					case 0xfe8:
						data = peripheral_id >> 8;
						break;
					case 0xfec:
						data = peripheral_id;
						break;
					case 0xff0:
						data = primecell_id >> 24;
						break;
					case 0xff4:
						data = primecell_id >> 16;
						break;
					case 0xff8:
						data = primecell_id >> 8;
						break;
					case 0xffc:
						data = primecell_id;
						break;
					default:
						LC_WARNING(LogGenericPrimecell) << "Attempted read to unimplemented register " << std::hex << reg << " at device " << GetBaseAddress();
						data = 0;
						return false;
				}

				data &= 0xff;
				return true;
			}

			bool GenericPrimecellDevice::Write(uint32_t offset, uint8_t size, uint32_t data)
			{
				uint32_t reg = (offset & (GetSize()-1));
				switch(reg) {
					default:
						LC_WARNING(LogGenericPrimecell) << "Attempted write to unimplemented register " << std::hex << reg << " at device " << GetBaseAddress();
				}
				return false;
			}
		}
	}
}
