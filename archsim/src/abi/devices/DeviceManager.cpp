#include <cassert>

#include "abi/devices/DeviceManager.h"
#include "abi/devices/Component.h"

#include "translate/profile/RegionArch.h"
#include "util/LogContext.h"

UseLogContext(LogDevice)

using namespace archsim::abi::devices;
using namespace archsim::abi::memory;

DeviceManager::DeviceManager()
	: min_device_address(0)
{

}

DeviceManager::~DeviceManager()
{
	// FIXME: some devices should not be deleted like this!
	// (does the device manager really own the devices?)
	//for(auto dev : device_set) delete dev;
}

bool DeviceManager::InstallDevice(guest_addr_t base_address, guest_size_t device_size, MemoryComponent& device)
{
	assert(base_address > 0);
	assert(device_size == device.GetSize());
	assert(archsim::translate::profile::RegionArch::PageOffsetOf(base_address) == 0);

	// Check first: does this device overlap any others?
	for(guest_addr_t addr = base_address; addr < base_address+device_size; addr += archsim::translate::profile::RegionArch::PageSize) {
		if(device_bitmap[archsim::translate::profile::RegionArch::PageIndexOf(addr)]) {
			LC_ERROR(LogDevice) << "Could not map device on page " << std::hex << addr << ": Devices overlap.";
			return false;
		}
	}

	if (min_device_address == 0 || base_address < min_device_address)
		min_device_address = base_address;

	device_set.insert(&device);
	devices[base_address] = &device;

	for(guest_addr_t addr = base_address; addr < base_address+device_size; addr += archsim::translate::profile::RegionArch::PageSize) {
		device_bitmap.set(archsim::translate::profile::RegionArch::PageIndexOf(addr));
	}

	return true;
}

bool DeviceManager::LookupDevice(memory::guest_addr_t device_address, MemoryComponent*& device)
{
	for(auto device_i : devices) {
		if((device_address >= device_i.first) && (device_address < (device_i.first + device_i.second->GetSize()))) {
			device = device_i.second;
			return true;
		}
	}

	return false;

//	device_map_t::iterator i = devices.upper_bound(device_address);
//	if(i == devices.begin()) return false;
//
//	i--;
//
//	device = i->second;
//	if((i->first + device->GetSize()) < device_address) return false;
//	return true;

}
