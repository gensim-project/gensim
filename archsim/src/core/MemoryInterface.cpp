/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/MemoryInterface.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"

using namespace archsim;

DeclareLogContext(LogCacheMemory, "CachedMemory");

MemoryDevice::~MemoryDevice() {}

TranslationResult MMUTranslationProvider::Translate(Address virt_addr, Address& phys_addr, bool is_write, bool is_fetch, bool side_effects)
{
	AccessInfo info;
	info.Fetch = is_fetch;
	info.SideEffects = side_effects;
	info.Write = is_write;
	info.Kernel = thread_->GetExecutionRing() != 0;

	auto result = mmu_->Translate(thread_, virt_addr, phys_addr, info);
	switch(result) {
		case archsim::abi::devices::MMU::TXLN_OK:
			return TranslationResult::OK;
		default:
			return TranslationResult::NotPresent;
	}
}

MemoryResult MemoryInterface::WriteString(Address address, const char *data)
{
	do {
		Write8(address, *data);
		data++;
		address += 1;
	} while(*data);

	return MemoryResult::OK;
}

MemoryResult MemoryInterface::ReadString(Address address, char *data, size_t max_size)
{
	if(max_size == 0) {
		return MemoryResult::OK;
	}

	uint8_t cdata;
	do {
		Read8(address, cdata);
		*data = cdata;

		data++;
		address += 1;
		max_size--;
	} while(cdata && max_size != 0);

	return MemoryResult::OK;
}

MemoryResult MemoryInterface::Read(Address address, unsigned char *data, size_t size)
{
	while(size >= 8) {
		auto result = Read64(address, *(uint64_t*)data);

		if(result != MemoryResult::OK) {
			return result;
		}

		address += 8;
		data += 8;
		size -= 8;
	}
	while(size >= 4) {
		auto result = Read32(address, *(uint32_t*)data);

		if(result != MemoryResult::OK) {
			return result;
		}

		address += 4;
		data += 4;
		size -= 4;
	}
	for(unsigned int i = 0; i < size; ++i) {
		auto result = Read8(address + i, data[i]);
		if(result != MemoryResult::OK) {
			return result;
		}
	}

	return MemoryResult::OK;
}

MemoryResult MemoryInterface::Write(Address address, const unsigned char *data, size_t size)
{
	for(unsigned int i = 0; i < size; ++i) {
		Write8(address + i, data[i]);
	}

	return MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read8(Address address, uint8_t& data)
{
	return mem_model_.Read8(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read16(Address address, uint16_t& data)
{
	return mem_model_.Read16(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read32(Address address, uint32_t& data)
{
	return mem_model_.Read32(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read64(Address address, uint64_t& data)
{
	return mem_model_.Read64(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read128(Address address, uint128_t& data)
{
	return mem_model_.Read128(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write8(Address address, uint8_t data)
{
	return mem_model_.Write8(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write16(Address address, uint16_t data)
{
	return mem_model_.Write16(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write32(Address address, uint32_t data)
{
	return mem_model_.Write32(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write64(Address address, uint64_t data)
{
	return mem_model_.Write64(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write128(Address address, uint128_t data)
{
	return mem_model_.Write128(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read8(Address address, uint8_t& data)
{
	return mem_model_.Fetch8(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read16(Address address, uint16_t& data)
{
	return mem_model_.Fetch16(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read32(Address address, uint32_t& data)
{
	return mem_model_.Fetch32(address, data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read64(Address address, uint64_t& data)
{
	UNIMPLEMENTED;
}
MemoryResult LegacyFetchMemoryInterface::Read128(Address address, uint128_t& data)
{
	UNIMPLEMENTED;
}

MemoryResult LegacyFetchMemoryInterface::Write8(Address address, uint8_t data)
{
	UNIMPLEMENTED;
}

MemoryResult LegacyFetchMemoryInterface::Write16(Address address, uint16_t data)
{
	UNIMPLEMENTED;
}

MemoryResult LegacyFetchMemoryInterface::Write32(Address address, uint32_t data)
{
	UNIMPLEMENTED;
}

MemoryResult LegacyFetchMemoryInterface::Write64(Address address, uint64_t data)
{
	UNIMPLEMENTED;
}

MemoryResult LegacyFetchMemoryInterface::Write128(Address address, uint128_t data)
{
	UNIMPLEMENTED;
}

CachedLegacyMemoryInterface::CachedLegacyMemoryInterface(int index, archsim::abi::memory::MemoryModel& mem_model, archsim::core::thread::ThreadInstance* thread) : mem_model_(mem_model), thread_(thread)
{
	cache_offset_ = thread->GetStateBlock().AddBlock("memory_cache_" + std::to_string(index), sizeof (struct Cache));
	Invalidate();
}

void CachedLegacyMemoryInterface::Invalidate()
{
	for(auto &i : GetCache()->cache) {
		i.tag = Address(1);
		i.data = nullptr;
	}
}

CachedLegacyMemoryInterface::Cache* CachedLegacyMemoryInterface::GetCache()
{
	return (Cache*)(((char*)thread_->GetStateBlock().GetData()) + cache_offset_);
}


void CachedLegacyMemoryInterface::LoadEntryFor(struct CacheEntry *entry, Address addr)
{
	void *ptr;

	bool success = mem_model_.LockRegion(addr.PageBase(), Address::PageSize, ptr);
	if(!success) {
		throw std::logic_error("Memory error.");
	}

	entry->data = ptr;
	entry->tag = addr.PageBase();
}

void* CachedLegacyMemoryInterface::GetPtr(Address addr)
{
	struct Cache *cache = GetCache();

	// get cache index
	uint32_t index = addr.GetPageIndex() % Cache::kCacheSize;

	auto &entry = cache->cache[index];
	if(entry.tag != addr.PageBase()) {
//		LC_DEBUG1(LogCacheMemory) << "Cache miss: loading for " << addr;
		LoadEntryFor(&entry, addr);
	}

	auto ptr = (void*)(((char*)entry.data) + addr.GetPageOffset());

//	LC_DEBUG1(LogCacheMemory) << "Got " << (void*)ptr << " for " << addr;
	return ptr;
}

MemoryResult CachedLegacyMemoryInterface::Read8(Address address, uint8_t& data)
{
	data = *(uint8_t*)GetPtr(address);
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Read16(Address address, uint16_t& data)
{
	if(address.GetPageIndex() != (address + 1).GetPageIndex()) {
		return Read(address, (char*)&data, 2);
	}
	data = *(uint16_t*)GetPtr(address);
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Read32(Address address, uint32_t& data)
{
	if(address.GetPageIndex() != (address + 3).GetPageIndex()) {
		return Read(address, (char*)&data, 4);
	}
	data = *(uint32_t*)GetPtr(address);
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Read64(Address address, uint64_t& data)
{
	if(address.GetPageIndex() != (address + 7).GetPageIndex()) {
		return Read(address, (char*)&data, 8);
	}
	data = *(uint64_t*)GetPtr(address);
	return MemoryResult::OK;
}
MemoryResult CachedLegacyMemoryInterface::Read128(Address address, uint128_t& data)
{
	return Read(address, (char*)&data, 16);
}

MemoryResult CachedLegacyMemoryInterface::Write8(Address address, uint8_t data)
{
	*(uint8_t*)GetPtr(address) = data;
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Write16(Address address, uint16_t data)
{
	if(address.GetPageIndex() != (address + 1).GetPageIndex()) {
		return Write(address, (char*)&data, 2);
	}
	*(uint16_t*)GetPtr(address) = data;
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Write32(Address address, uint32_t data)
{
	if(address.GetPageIndex() != (address + 3).GetPageIndex()) {
		return Write(address, (char*)&data, 4);
	}
	*(uint32_t*)GetPtr(address) = data;
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Write64(Address address, uint64_t data)
{
	if(address.GetPageIndex() != (address + 7).GetPageIndex()) {
		return Write(address, (char*)&data, 8);
	}
	*(uint64_t*)GetPtr(address) = data;
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Write128(Address address, uint128_t data)
{
	return Write(address, (char*)&data, 16);
}

MemoryResult CachedLegacyMemoryInterface::Read(Address addr, char* data, size_t len)
{
	for(int i = 0; i < len; ++i) {
		Read8(addr + i, *(uint8_t*)(data + i));
	}
	return MemoryResult::OK;
}

MemoryResult CachedLegacyMemoryInterface::Write(Address addr, const char* data, size_t len)
{
	for(int i = 0; i < len; ++i) {
		Write8(addr + i, *(uint8_t*)(data + i));
	}
	return MemoryResult::OK;
}
