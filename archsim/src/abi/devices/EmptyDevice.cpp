/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/EmptyDevice.h"

using namespace archsim::abi::devices;

EmptyDevice::EmptyDevice(EmulationModel &parent, Address base_address, uint64_t size) : MemoryComponent(parent, base_address, size)
{

}

bool EmptyDevice::Initialise()
{
	return true;
}

bool EmptyDevice::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	data = 0;
	return true;
}
bool EmptyDevice::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	return true;
}