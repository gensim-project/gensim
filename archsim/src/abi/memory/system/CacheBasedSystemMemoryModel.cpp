/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * SystemMemoryModel.cpp
 *
 *  Created on: 9 Jul 2014
 *      Author: harry
 */
#include "define.h"
#include "system.h"

#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/SystemEmulationModel.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/Component.h"
#include "abi/devices/MMU.h"
#include "translate/TranslationManager.h"
#include "translate/profile/Region.h"

#include "util/LogContext.h"

UseLogContext(LogSystemMemoryModel);


using archsim::Address;
using namespace archsim::abi::memory;
using namespace archsim::translate::profile;

RegisterComponent(SystemMemoryModel, CacheBasedSystemMemoryModel, "cache", "Cache based system memory model", MemoryModel*, archsim::util::PubSubContext *);

#define SIGSEGV_INVALIDATE 0

SMMCacheEntry::SMMCacheEntry() : virt_tag(kInvalidTag), data(NULL), phys_addr(0) {}
SMMCacheEntry::SMMCacheEntry(guest_addr_t tag, Address phys_addr, void *data, flag_t flags) : virt_tag(tag), phys_addr(phys_addr | (flags & kFlagsMask)), data(data) {}

SMMCache::SMMCache() : pointertaken(false), dirty(true)
{
#if SIGSEGV_INVALIDATE
	read_cache = (SMMCacheEntry*)mmap(NULL, GetReadCacheSize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	write_cache = (SMMCacheEntry*)mmap(NULL, GetWriteCacheSize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#else
	dirty = true;
	_cache = (SMMCacheEntry*)mmap(NULL, sizeof(SMMCacheEntry)*kCacheSize, PROT_READ|PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	_dirty_pages.set();
	Flush();
#endif
}

SMMCache::~SMMCache()
{
	munmap(_cache, sizeof(SMMCacheEntry)*kCacheSize);
//	free(read_cache);
//	free(write_cache);
}

void SMMCache::Flush()
{
	if(!dirty && !pointertaken) return;

	LC_DEBUG1(LogSystemMemoryModel) << "Flushing SMM Cache";


	dirty = false;

#if SIGSEGV_INVALIDATE
	mprotect(read_cache, GetReadCacheSize(), PROT_NONE);
	mprotect(write_cache, GetWriteCacheSize(), PROT_NONE);
#else

	for(uint32_t i = 0; i < _dirty_pages.size(); ++i) {
		if(_dirty_pages.test(i)) {
			for(uint32_t j = 0; j < kCachePageSize; ++j) {
				uint32_t idx = j + (kCachePageSize*i);
				_cache[idx].Invalidate();
			}
		}
	}

	_dirty_pages.reset();

//	for(uint32_t i = 0; i < kCacheSize; ++i) {
//		_cache[i].Invalidate();
//	}

#endif
}

void SMMCache::Evict(guest_addr_t addr)
{
	_cache[addr.GetPageIndex() % kCacheSize].Invalidate();
}

CacheBasedSystemMemoryModel::CacheBasedSystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub) : SystemMemoryModel(phys_mem, pubsub), translation_model(NULL)
{

}

CacheBasedSystemMemoryModel::~CacheBasedSystemMemoryModel()
{
}

static void FlushCallback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	SystemMemoryModel *smm = (SystemMemoryModel*)ctx;
	CacheBasedSystemMemoryModel *cmm = (CacheBasedSystemMemoryModel*)smm;
	switch(type) {
		case PubSubType::ITlbEntryFlush:
		case PubSubType::DTlbEntryFlush:
			cmm->EvictCacheEntry(Address((uint64_t)data));
			break;
		case PubSubType::RegionDispatchedForTranslationPhysical:
		case PubSubType::ITlbFullFlush:
		case PubSubType::DTlbFullFlush:
			cmm->FlushCaches();
			break;
		case PubSubType::PrivilegeLevelChange:
			cmm->InstallCaches();
			break;
		default:
			assert(false);
	}
}

bool CacheBasedSystemMemoryModel::Initialise()
{
#if CONFIG_LLVM
	translation_model = new CacheBasedSystemMemoryTranslationModel();
	translation_model->Initialise(*this);
#endif

	subscriber = new archsim::util::PubSubscriber(GetThread()->GetEmulationModel().GetSystem().GetPubSub());
	subscriber->Subscribe(PubSubType::RegionDispatchedForTranslationPhysical, FlushCallback, this);
	subscriber->Subscribe(PubSubType::DTlbEntryFlush, FlushCallback, this);
	subscriber->Subscribe(PubSubType::DTlbFullFlush, FlushCallback, this);
	subscriber->Subscribe(PubSubType::PrivilegeLevelChange, FlushCallback, this);

	InstallCaches();
	FlushCaches();
	return true;
}

void CacheBasedSystemMemoryModel::Destroy()
{

}

void CacheBasedSystemMemoryModel::FlushCaches()
{
	for(auto p : caches_) {
		auto &caches = p.second;
		if(caches.first != nullptr) {
			caches.first->Flush();
		}
		if(caches.second != nullptr) {
			caches.second->Flush();
		}
	}
}

void CacheBasedSystemMemoryModel::EvictCacheEntry(Address virt_addr)
{
	for(auto p : caches_) {
		auto &caches = p.second;
		if(caches.first != nullptr) {
			caches.first->Evict(virt_addr);
		}
		if(caches.second != nullptr) {
			caches.second->Evict(virt_addr);
		}
	}
}

MemoryTranslationModel &CacheBasedSystemMemoryModel::GetTranslationModel()
{
#if CONFIG_LLVM
	return *translation_model;
#else
	abort();
#endif
}

uint32_t CacheBasedSystemMemoryModel::DoRead(guest_addr_t virt_addr, uint8_t *data, int size, bool use_perms, bool isFetch)
{
	bool unaligned = false;
	if(UNLIKELY(archsim::options::MemoryCheckAlignment)) {
		if(virt_addr.Get() & (size-1)) {
			unaligned = true;
		}
	}
	if(unaligned) {
		for(int i = 0; i < size; ++i) {
			uint32_t rval = DoRead(virt_addr+i, data+i, 1, use_perms, isFetch);
			if(rval) return rval;
		}
		return 0;
	}

	archsim::VirtualAddress va (virt_addr.Get());

	SMMCacheEntry *entry;
	bool hit = true;

	auto &cache = GetCache(0, 3, false);
	hit = cache.TryGetEntry(virt_addr, entry);

	uint32_t rc = 0;
	if(UNLIKELY(!hit)) {
		if((rc = UpdateCacheEntry(virt_addr, entry, false, isFetch, true))) {
			return rc;
		}

		// We might still have failed to fill in the cache entry, if this is a device access
		if(entry->IsDevice()) {
			abi::devices::MemoryComponent* device = entry->GetDevice();
			LC_DEBUG2(LogSystemMemoryModel) << "Memory read from device at VA " << std::hex << virt_addr << " PA base " << std::hex << entry->GetDevice()->GetBaseAddress();
			virt_addr &= device->GetSize()-1;
			uint64_t zdata;
			if(device->Read(virt_addr.Get(), size, zdata)) {
				switch(size) {
					case 1:
						*data = zdata;
						break;
					case 2:
						*(uint16_t*)data = zdata;
						break;
					case 4:
						*(uint32_t*)data = zdata;
						break;
					case 8:
						*(uint64_t*)data = zdata;
						break;
					default:
						assert(false);
				}
			} else {
				memset(data, 0, size);
			}
			return 0;
		}
	}

	uint8_t *page_base = (uint8_t*)entry->GetMemory();
	switch(size) {
		case 1:
			*data = *(page_base + va.GetPageOffset());
			break;
		case 2:
			*(uint16_t*)data = *(uint16_t*)(page_base + va.GetPageOffset());
			break;
		case 4:
			*(uint32_t*)data = *(uint32_t*)(page_base + va.GetPageOffset());
			break;
		case 8:
			*(uint64_t*)data = *(uint64_t*)(page_base + va.GetPageOffset());
			break;
		default:
			assert(false);
	}

	return 0;
}

void CacheBasedSystemMemoryModel::InstallCaches()
{
	auto &stateblock = GetThread()->GetStateBlock();
	for(auto &mode : caches_) {
		std::string read_name = "mem_cache_" + std::to_string(mode.first.first) + "_" + std::to_string(mode.first.second) + "_read";
		std::string write_name = "mem_cache_" + std::to_string(mode.first.first) + "_" + std::to_string(mode.first.second) + "_write";

		if(!stateblock.GetDescriptor().HasEntry(read_name)) {
			stateblock.AddBlock(read_name, 8);
		}
		if(!stateblock.GetDescriptor().HasEntry(write_name)) {
			stateblock.AddBlock(write_name, 8);
		}
		GetThread()->GetStateBlock().SetEntry<void*>(read_name, GetCache(mode.first.first, mode.first.second, 0).GetCachePtr());
		GetThread()->GetStateBlock().SetEntry<void*>(write_name,GetCache(mode.first.first, mode.first.second, 1).GetCachePtr());

	}
}

uint32_t CacheBasedSystemMemoryModel::Read(guest_addr_t virt_addr, uint8_t *data, int size)
{
#ifdef CONFIG_MEMORY_EVENTS
	RaiseEvent(MemoryModel::MemEventRead, virt_addr, size);
#endif
	return DoRead(virt_addr, data, size, true, false);
}

uint32_t CacheBasedSystemMemoryModel::Fetch(guest_addr_t virt_addr, uint8_t *data, int size)
{
	return DoRead(virt_addr, data, size, true, true);
}

uint32_t CacheBasedSystemMemoryModel::Write(guest_addr_t virt_addr, uint8_t *data, int size)
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
			case 8:
				if(virt_addr.Get() & 0x7) unaligned = true;
				break;
		}
	}
	if(unaligned) {
		for(int i = 0; i < size; ++i) {
			uint32_t rval = Write(virt_addr+i, data+i, 1);
			if(rval) return rval;
		}
		return 0;
	}

#ifdef CONFIG_MEMORY_EVENTS
	RaiseEvent(MemoryModel::MemEventWrite, virt_addr, size);
#endif

	archsim::VirtualAddress va (virt_addr.Get());

	SMMCacheEntry *entry;
	bool hit = true;
	auto &cache = GetCache(0, 3, true);
	hit = cache.TryGetEntry(virt_addr, entry);

	// If we missed in the cache, try and fill in the cache entry
	uint32_t rc = 0;
	if(UNLIKELY(!hit)) {
		if((rc = UpdateCacheEntry(virt_addr, entry, true, false, true))) {
			return rc;
		}

		// We might still have failed to fill in the cache entry, if this is a device access
		if(entry->IsDevice()) {
			abi::devices::MemoryComponent* device = entry->GetDevice();
			virt_addr &= device->GetSize()-1;
			switch(size) {
				case 1:
					device->Write(virt_addr.Get(), size, *data);
					break;
				case 2:
					device->Write(virt_addr.Get(), size, *(uint16_t*)data);
					break;
				case 4:
					device->Write(virt_addr.Get(), size, *(uint32_t*)data);
					break;
				case 8:
					device->Write(virt_addr.Get(), size, *(uint64_t*)data);
					break;
				default:
					assert(false);
			}
			return 0;
		}
	}

	if (GetCodeRegions().IsRegionCode(PhysicalAddress(entry->GetPhysAddr().Get()))) {
		GetCodeRegions().InvalidateRegion(PhysicalAddress(entry->GetPhysAddr().Get()));
	}

	uint8_t *page_base = (uint8_t*)entry->GetMemory();
	switch(size) {
		case 1:
			*(page_base + va.GetPageOffset()) = *data;
			break;
		case 2:
			*(uint16_t*)(page_base + va.GetPageOffset()) = *(uint16_t*)data;
			break;
		case 4:
			*(uint32_t*)(page_base + va.GetPageOffset()) = *(uint32_t*)data;
			break;
		case 8:
			*(uint64_t*)(page_base + va.GetPageOffset()) = *(uint64_t*)data;
			break;
		default:
			assert(false);
	}

	return 0;
}

uint32_t CacheBasedSystemMemoryModel::Peek(guest_addr_t virt_addr, uint8_t *data, int size)
{
	guest_addr_t phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), virt_addr, phys_addr, MMUACCESSINFO(GetThread()->GetExecutionRing(), 0, 0));

	if (UNLIKELY(rc)) {
		return rc;
	} else {
		return GetPhysMem()->Peek(phys_addr, data, size);
	}
}

uint32_t CacheBasedSystemMemoryModel::Poke(guest_addr_t virt_addr, uint8_t *data, int size)
{
	guest_addr_t phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), virt_addr, phys_addr, MMUACCESSINFO(GetThread()->GetExecutionRing(), 1, 0));

	if (UNLIKELY(rc)) {
		return rc;
	} else {
		if (GetCodeRegions().IsRegionCode(PhysicalAddress(phys_addr.Get()))) {
			GetCodeRegions().InvalidateRegion(PhysicalAddress(phys_addr.Get()));
		}

		return GetPhysMem()->Poke(phys_addr, data, size);
	}
}

uint32_t CacheBasedSystemMemoryModel::Read8User(guest_addr_t guest_addr, uint32_t &data)
{
	guest_addr_t phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), guest_addr, phys_addr, MMUACCESSINFO_SE(0,0,0));

	if(UNLIKELY(rc)) return rc;
	else return GetPhysMem()->Read8_zx(phys_addr, data);
}

uint32_t CacheBasedSystemMemoryModel::Read32User(guest_addr_t guest_addr, uint32_t& data)
{
	guest_addr_t phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), guest_addr, phys_addr, MMUACCESSINFO_SE(0,0,0));

	if(UNLIKELY(rc)) return rc;
	else return GetPhysMem()->Read32(phys_addr, data);
}

uint32_t CacheBasedSystemMemoryModel::Write8User(guest_addr_t guest_addr, uint8_t data)
{
	guest_addr_t phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), guest_addr, phys_addr, MMUACCESSINFO_SE(0,1,0));

	if(UNLIKELY(rc)) return rc;
	else {
		if (GetCodeRegions().IsRegionCode(PhysicalAddress(phys_addr.Get()))) {
			GetCodeRegions().InvalidateRegion(PhysicalAddress(phys_addr.Get()));
		}
		return GetPhysMem()->Write8(phys_addr, data);
	}
}

uint32_t CacheBasedSystemMemoryModel::Write32User(guest_addr_t guest_addr, uint32_t data)
{
	guest_addr_t phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), guest_addr, phys_addr, MMUACCESSINFO_SE(0,1,0));

	if(UNLIKELY(rc)) return rc;
	else {
		if (GetCodeRegions().IsRegionCode(PhysicalAddress(phys_addr.Get()))) {
			GetCodeRegions().InvalidateRegion(PhysicalAddress(phys_addr.Get()));
		}
		return GetPhysMem()->Write32(phys_addr, data);
	}
}

bool CacheBasedSystemMemoryModel::ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr)
{
	return false;
}

uint32_t CacheBasedSystemMemoryModel::PerformTranslation(Address virt_addr, Address &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info)
{
	SMMCacheEntry *entry;
	auto &cache = GetCache(0, info.Ring, info.Write);
	if(cache.TryGetEntry(virt_addr, entry)) {
		out_phys_addr = (entry->GetPhysAddr() & ~archsim::Address::PageMask) | (virt_addr & archsim::Address::PageMask);
		return 0;
	}

	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), virt_addr, out_phys_addr, info);
	return rc;
}

uint32_t CacheBasedSystemMemoryModel::UpdateCacheEntry(guest_addr_t addr, SMMCacheEntry *entry, bool isWrite, bool isFetch, bool allow_side_effects)
{
	Address phys_addr;
	uint32_t rc = (uint32_t)GetMMU()->Translate(GetThread(), addr, phys_addr, MMUACCESSINFO2(GetThread()->GetExecutionRing(), isWrite,isFetch, allow_side_effects));
	if(rc) {
		entry->Invalidate();
		return rc;
	}

	guest_addr_t virt_page_base = addr.PageBase();

	if(GetDeviceManager()->HasDevice(phys_addr)) {
		abi::devices::MemoryComponent *device = NULL;
		GetDeviceManager()->LookupDevice(phys_addr, device);
		assert(device != NULL);

		*entry = SMMCacheEntry(Address(SMMCacheEntry::kInvalidTag), Address(0), device, SMMCacheEntry::kIsDevice);
		return 0;
	} else {
		host_addr_t host_page_addr;
		guest_addr_t phys_page_base = phys_addr.PageBase();
		if(!GetPhysMem()->LockRegion(phys_page_base, archsim::translate::profile::RegionArch::PageSize, host_page_addr)) {
			return 1;
		}
		*entry = SMMCacheEntry(virt_page_base, phys_page_base, (void*)((uint64_t)host_page_addr), 0);
	}

	return 0;
}

SMMCache &CacheBasedSystemMemoryModel::GetCache(int interface, int mode, bool write_cache)
{
	auto &p = caches_[ {interface, mode}];
	if(write_cache) {
		if(p.second == nullptr) {
			p.second = new SMMCache();
			InstallCaches();
		}
		return *p.second;
	} else {
		if(p.first == nullptr) {
			p.first = new SMMCache();
			InstallCaches();
		}
		return *p.first;
	}
}
