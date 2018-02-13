/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"
#include "genC/ssa/testing/MachineState.h"

#include <ostream>
#include <iomanip>
#include <sstream>

using namespace gensim::genc::ssa::testing;

void MachineStateInterface::Reset()
{
	RegisterFile().Reset();
	Memory().Reset();
}

bool MachineStateInterface::Equals(MachineStateInterface& other)
{
	return RegisterFile().Equals(other.RegisterFile()) && Memory().Equals(other.Memory());
}

void MemoryState::Reset()
{
	_data.clear();
}

bool MemoryState::Equals(MemoryState& other)
{
	return _data == other._data;
}

void MemoryState::Read(uint64_t addr, size_t size, uint8_t* data)
{
	for(unsigned i = 0; i < size; ++i) {
		data[i] = ReadByte(addr + i);
	}
}

void MemoryState::Write(uint64_t addr, size_t size, uint8_t* data)
{
	for(unsigned i = 0; i < size; ++i) {
		WriteByte(addr + i, data[i]);
	}
}

uint8_t MemoryState::ReadByte(uint64_t addr)
{
	if(_data.count(addr) == 0) {
		_data[addr] = (addr & 0xff) ^ ((addr >> 8) & 0xff);
	} 
	return _data[addr];
}

void MemoryState::WriteByte(uint64_t addr, uint8_t value)
{
	_data[addr] = value;
}

RegisterFileState::RegisterFileState() : _wrap(false)
{

}

BasicRegisterFileState::BasicRegisterFileState()
{

}


void BasicRegisterFileState::Read(uint32_t offset, size_t size, uint8_t* data)
{
	for(uint32_t i = 0; i < size; ++i) {
		data[i] = ReadByte(offset+i);
	}
}

void BasicRegisterFileState::Write(uint32_t offset, size_t size, const uint8_t* data)
{
	for(uint32_t i = 0; i < size; ++i) {
		WriteByte(offset+i, data[i]);
	}
}

void BasicRegisterFileState::Reset()
{
	data_.clear();
}

bool BasicRegisterFileState::IsSet(uint32_t offset)
{
	return data_.count(offset);
}


size_t BasicRegisterFileState::Size() const
{
	return size_;
}

void BasicRegisterFileState::SetSize(size_t size)
{
	size_ = size;
}

bool RegisterFileState::Equals(RegisterFileState& other)
{
	UNIMPLEMENTED;
}

uint8_t BasicRegisterFileState::ReadByte(uint32_t offset)
{
	if(!Wrap()) {
		if(offset >= size_) {
			throw std::logic_error("Register file access out of range");
		}
	}
	
	offset %= size_;
	
	if(!data_.count(offset)) {
		data_[offset] = offset & 0xff;
	}
	return data_.at(offset);
}

void BasicRegisterFileState::WriteByte(uint32_t offset, uint8_t data)
{
	if(!Wrap()) {
		if(offset >= size_) {
			throw std::logic_error("Register file access out of range");
		}
	}
	data_[offset % size_] = data;
}

bool BasicRegisterFileState::Equals(RegisterFileState& other)
{
	BasicRegisterFileState *otherstate = dynamic_cast<BasicRegisterFileState*>(&other);
	if(otherstate != nullptr) {
		return otherstate->data_ == data_;
	} else {
		UNIMPLEMENTED;
	}
}

std::string MachineStateInterface::Dump()
{
	std::ostringstream str;
	str << "Instruction: "<< std::endl << Instruction().Dump() << std::endl;
	str << "Regfile: " << std::endl << RegisterFile().Dump() << std::endl;
	str << "Memory: " << std::endl << Memory().Dump() << std::endl;
	return str.str();
}

std::string MemoryState::Dump()
{
	std::ostringstream str;
	for(auto i : _data) {
		str << "[" << std::hex << i.first << "] " << (uint64_t)i.second << std::endl;
	}
	return str.str();
}

std::string RegisterFileState::Dump()
{
	std::ostringstream str;
	for(unsigned i = 0; i < Size(); ++i) {
		if(IsSet(i)) {
			str << "[" << std::hex << i << "] " << (uint64_t)Read8(i) << std::endl;
		}
	}
	return str.str();
}
