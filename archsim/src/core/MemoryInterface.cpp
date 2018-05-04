/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/MemoryInterface.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim;

TranslationResult MMUTranslationProvider::Translate(Address virt_addr, Address& phys_addr, bool is_write, bool is_fetch, bool side_effects) {
	AccessInfo info;
	info.Fetch = is_fetch;
	info.SideEffects = side_effects;
	info.Write = is_write;
	info.Kernel = thread_->GetExecutionRing() != 0;
	uint32_t physaddr;
	
	auto result = mmu_->Translate(thread_, virt_addr.Get(), physaddr, info);
	phys_addr = Address(physaddr);
	switch(result) {
		case archsim::abi::devices::MMU::TXLN_OK: return TranslationResult::OK;
		default:
			return TranslationResult::NotPresent;
	}
}

MemoryResult MemoryInterface::WriteString(Address address, const char *data) {
	do {
		Write8(address, *data);
		data++;
		address += 1;
	} while(*data);
	
	return MemoryResult::OK;
}

MemoryResult MemoryInterface::ReadString(Address address, char *data, size_t max_size) {
	UNIMPLEMENTED;
}

MemoryResult MemoryInterface::Read(Address address, unsigned char *data, size_t size) {
	for(unsigned int i = 0; i < size; ++i) {
		auto result = Read8(address + i, data[i]);
		if(result != MemoryResult::OK) {
			return result;
		}
	}
	
	return MemoryResult::OK; 
}

MemoryResult MemoryInterface::Write(Address address, const unsigned char *data, size_t size) {
	for(unsigned int i = 0; i < size; ++i) {
		Write8(address + i, data[i]);
	}
	
	return MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read8(Address address, uint8_t& data)
{
	return mem_model_.Read8(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read16(Address address, uint16_t& data)
{
	return mem_model_.Read16(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read32(Address address, uint32_t& data)
{
	return mem_model_.Read32(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Read64(Address address, uint64_t& data)
{
	return mem_model_.Read64(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write8(Address address, uint8_t data)
{
	return mem_model_.Write8(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write16(Address address, uint16_t data)
{
	return mem_model_.Write16(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write32(Address address, uint32_t data)
{
	return mem_model_.Write32(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyMemoryInterface::Write64(Address address, uint64_t data)
{
	return mem_model_.Write64(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read8(Address address, uint8_t& data)
{
	return mem_model_.Fetch8(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read16(Address address, uint16_t& data)
{
	return mem_model_.Fetch16(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read32(Address address, uint32_t& data)
{
	return mem_model_.Fetch32(address.Get(), data) ? MemoryResult::Error : MemoryResult::OK;
}

MemoryResult LegacyFetchMemoryInterface::Read64(Address address, uint64_t& data)
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