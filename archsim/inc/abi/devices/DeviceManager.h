/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   DeviceManager.h
 * Author: s0457958
 *
 * Created on 10 July 2014, 13:51
 */

#ifndef DEVICEMANAGER_H
#define	DEVICEMANAGER_H

#include "abi/memory/MemoryModel.h"
#include "translate/profile/RegionArch.h"

#include <bitset>
#include <map>
#include <set>

namespace archsim
{
	namespace abi
	{
		class SystemEmulationModel;

		namespace devices
		{
			class MemoryComponent;

			class DeviceManager
			{
			public:
				DeviceManager();
				~DeviceManager();

				bool InstallDevice(memory::guest_addr_t base_address, memory::guest_size_t device_size, MemoryComponent& device);
				bool LookupDevice(memory::guest_addr_t device_address, MemoryComponent*& device);
				bool HasDevice(memory::guest_addr_t device_address)
				{
					return device_bitmap.test(device_address.GetPageIndex());
				}

			private:
				memory::guest_addr_t min_device_address;

				typedef std::map<memory::guest_addr_t, MemoryComponent*> device_map_t;
				device_map_t devices;

				//Keep also a set of registered devices, since some devices may be multiply registered
				std::set<MemoryComponent*> device_set;

				std::bitset<archsim::translate::profile::RegionArch::PageCount> device_bitmap;
			};
		}
	}
}

#endif	/* DEVICEMANAGER_H */

