/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * BaseSystemMemoryModel.cpp
 *
 *  Created on: 22 Apr 2015
 *      Author: harry
 */

#include "abi/memory/system/BaseSystemMemoryModel.h"
#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/Component.h"
#include "abi/devices/MMU.h"
#include "util/LogContext.h"
#include "stdio.h"

using namespace archsim::abi::memory;

DefineComponentType(SystemMemoryModel, MemoryModel*, archsim::util::PubSubContext*);
RegisterComponent(SystemMemoryModel, BaseSystemMemoryModel, "base", "Basic naive memory model", MemoryModel*, archsim::util::PubSubContext *);

UseLogContext(LogMemoryModel);
DeclareChildLogContext(LogSystemMemoryModel, LogMemoryModel, "System");

BaseSystemMemoryModel::BaseSystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub) : SystemMemoryModel(phys_mem, pubsub), txln_model(NULL), _unaligned_behaviour(Unaligned_EMULATE)
{
}

bool BaseSystemMemoryModel::Initialise()
{
	if(!GetThread()) return false;
	if(!GetDeviceManager()) return false;
	if(!GetPhysMem()) return false;
	if(!GetMMU()) return false;

#if CONFIG_LLVM
	txln_model = new BaseSystemMemoryTranslationModel();
#endif

	return true;
}

void BaseSystemMemoryModel::Destroy()
{

}

bool BaseSystemMemoryModel::ResolveGuestAddress(host_const_addr_t, guest_addr_t&)
{
	return false;
}

uint32_t BaseSystemMemoryModel::PerformTranslation(Address virt_addr, Address &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info)
{
	return GetMMU()->Translate(GetThread(), virt_addr, out_phys_addr, info);
}

MemoryTranslationModel &BaseSystemMemoryModel::GetTranslationModel()
{
#if CONFIG_LLVM
	return *txln_model;
#else
	abort();
#endif
}

uint32_t BaseSystemMemoryModel::DoRead(guest_addr_t virt_addr, uint8_t *data, int size, bool use_perms, bool side_effects, bool is_fetch)
{
	bool unaligned = false;
	if(UNLIKELY(archsim::options::MemoryCheckAlignment)) {
		switch(size) {
			case 2:
				if(virt_addr.Get() & 0x1) unaligned = true;
				break;
			case 4:
				if(virt_addr.Get() & 0x3) unaligned = true;
				break;
		}
	}
	if(unaligned) {
		if(_unaligned_behaviour == Unaligned_TRAP) {
			assert(false);
		} else {
			for(int i = 0; i < size; ++i) {
				uint32_t rval = DoRead(virt_addr+i, data+i, 1, use_perms, side_effects, is_fetch);
				if(rval) return rval;
			}
			return 0;
		}
	}

	Address phys_addr;
	uint32_t fault = GetMMU()->Translate(GetThread(), virt_addr, phys_addr, MMUACCESSINFO2(use_perms ? GetThread()->GetExecutionRing() : false, false, is_fetch, side_effects));

	LC_DEBUG4(LogSystemMemoryModel) << "DoRead Fault: " << fault;

	if(fault) return fault;

	if(GetDeviceManager()->HasDevice(phys_addr)) {
		devices::MemoryComponent *dev;
		GetDeviceManager()->LookupDevice(phys_addr, dev);

		uint32_t device_data;
		uint32_t mask = dev->GetSize()-1;

		LC_DEBUG2(LogSystemMemoryModel) << "Performing device read from address V" << std::hex << virt_addr << "(P" << phys_addr << ")";
		LC_DEBUG2(LogSystemMemoryModel) << "Hit device " << std::hex << (uint64_t)dev << " at address: " << std::hex << dev->GetBaseAddress();

		Address device_addr = virt_addr & mask;
		if (!dev->Read(device_addr.Get(), size, device_data)) { //*(uint32_t*)(data)))
			LC_DEBUG2(LogSystemMemoryModel) << "Read failed!";
			memset(data, 0, size);
		} else {
			switch (size) {
				case 1:
					*data = (uint8_t)device_data;
					break;
				case 2:
					*((uint16_t *)data) = (uint16_t)device_data;
					break;
				case 4:
					*((uint32_t *)data) = device_data;
					break;
				default:
					assert(false && "Invalid device read size");
			}

			LC_DEBUG2(LogSystemMemoryModel) << "Read got data " << std::hex << device_data;
		}
		return 0;
	} else {
		switch(size) {
			case 1:
				return GetPhysMem()->Read8(phys_addr, *data);
			case 2:
				return GetPhysMem()->Read16(phys_addr, *(uint16_t*)data);
			case 4:
				return GetPhysMem()->Read32(phys_addr, *(uint32_t*)data);
			default:
				return GetPhysMem()->Read(phys_addr, data, size);
		}
	}
}

uint32_t BaseSystemMemoryModel::DoWrite(guest_addr_t virt_addr, uint8_t *data, int size, bool use_perms, bool side_effects)
{
	bool unaligned = false;
	if(UNLIKELY(archsim::options::MemoryCheckAlignment)) {
		switch(size) {
			case 2:
				if(virt_addr.Get() & 0x1) unaligned = true;
				break;
			case 4:
				if(virt_addr.Get() & 0x3) unaligned = true;
				break;
		}
	}
	if(unaligned) {
		if(_unaligned_behaviour == Unaligned_TRAP) {
			throw std::logic_error("An unaligned memory access caused a trap");
		} else {
			for(int i = 0; i < size; ++i) {
				uint32_t rval = DoWrite(virt_addr+i, data+i, 1, use_perms, side_effects);
				if(rval) return rval;
			}
			return 0;
		}
	}

	Address phys_addr;
	uint32_t fault = GetMMU()->Translate(GetThread(), virt_addr, phys_addr, MMUACCESSINFO2(use_perms ? GetThread()->GetExecutionRing() : false, true, 0, side_effects));

	LC_DEBUG4(LogSystemMemoryModel) << "DoWrite Fault: " << fault;

	if(fault) return fault;


	if(GetDeviceManager()->HasDevice(phys_addr)) {
		devices::MemoryComponent *dev;
		GetDeviceManager()->LookupDevice(phys_addr, dev);

		LC_DEBUG2(LogSystemMemoryModel) << "Performing device write to address V" << std::hex << virt_addr << "(P" << phys_addr << ")";
		LC_DEBUG2(LogSystemMemoryModel) << "Hit device " << std::hex << (uint64_t)dev;

		uint32_t device_data = 0;
		uint32_t mask = dev->GetSize()-1;

		virt_addr = virt_addr & mask;

		switch(size) {
			case 1: {
				LC_DEBUG2(LogSystemMemoryModel) << "Writing data " << std::hex << (uint32_t)*data;
				device_data = *(data);
				dev->Write(virt_addr.Get(), 1, *data);
				break;
			}
			case 2: {
				LC_DEBUG2(LogSystemMemoryModel) << "Writing data " << std::hex << *(uint16_t*)data;
				device_data = *(uint16_t*)(data);
				dev->Write(virt_addr.Get(), 2, *(uint16_t*)data);
				break;
			}
			case 4: {
				LC_DEBUG2(LogSystemMemoryModel) << "Writing data " << std::hex << *(uint32_t*)data;
				device_data = *(uint32_t*)(data);
				dev->Write(virt_addr.Get(), 4, *(uint32_t*)data);
				break;
			}
			default:
				assert(false);
		}

		return 0;
	} else {
		switch(size) {
			case 1:
				GetPhysMem()->Write8(phys_addr, *data);
				break;
			case 2:
				GetPhysMem()->Write16(phys_addr, *(uint16_t*)data);
				break;
			case 4:
				GetPhysMem()->Write32(phys_addr, *(uint32_t*)data);
				break;
			default:
				GetPhysMem()->Write(phys_addr, data, size);
				break;
		}

		if (GetCodeRegions().IsRegionCode(PhysicalAddress(phys_addr.Get()))) {
			GetCodeRegions().InvalidateRegion(PhysicalAddress(phys_addr.Get()));
		}

		return 0;

	}
}

uint32_t BaseSystemMemoryModel::Read(guest_addr_t virt_addr, uint8_t *data, int size)
{
	return DoRead(virt_addr, data, size, true, true, false);
}

uint32_t BaseSystemMemoryModel::Write(guest_addr_t virt_addr, uint8_t *data, int size)
{
	uint32_t temp = DoWrite(virt_addr, data, size, true, true);
	return temp;
}

uint32_t BaseSystemMemoryModel::Fetch(guest_addr_t virt_addr, uint8_t *data, int size)
{
	return DoRead(virt_addr, data, size, true, true, true);
}

uint32_t BaseSystemMemoryModel::Peek(guest_addr_t virt_addr, uint8_t *data, int size)
{
	return DoRead(virt_addr, data, size, true, false, false);
}

uint32_t BaseSystemMemoryModel::Poke(guest_addr_t virt_addr, uint8_t *data, int size)
{
	return DoWrite(virt_addr, data, size, true, false);
}

uint32_t BaseSystemMemoryModel::Read8User(guest_addr_t guest_addr, uint32_t&data)
{
	return DoRead(guest_addr, (uint8_t*)&data, 1, false, true, false);
}

uint32_t BaseSystemMemoryModel::Read32User(guest_addr_t guest_addr, uint32_t&data)
{
	return DoRead(guest_addr, (uint8_t*)&data, 4, false, true, false);
}

uint32_t BaseSystemMemoryModel::Write8User(guest_addr_t guest_addr, uint8_t data)
{
	return DoWrite(guest_addr, &data, 1, false, true);
}

uint32_t BaseSystemMemoryModel::Write32User(guest_addr_t guest_addr, uint32_t data)
{
	return DoWrite(guest_addr, (uint8_t*)&data, 4, false, true);
}
