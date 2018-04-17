/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/MemoryInterface.h"

using namespace archsim;

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
