/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "abi/EmulationModel.h"
#include "gensim/ThreadInstance.h"
#include "gensim/MemoryInterface.h"

using namespace archsim;

ThreadInstance::ThreadInstance(const ArchDescriptor& arch, StateBlockDescriptor &state_block_desc, archsim::abi::EmulationModel &emu_model) : descriptor_(arch), state_block_(state_block_desc), emu_model_(emu_model), mode_id_(0), ring_id_(0), register_file_(arch.GetRegisterFileDescriptor()), peripherals_(*this)
{
	// Need to fill in structures based on arch descriptor info
	
	// 2. Memory interfaces
	for(auto &interface_descriptor : arch.GetMemoryInterfaceDescriptor().GetInterfaces()) {
		memory_interfaces_[interface_descriptor.second.GetName()] = new MemoryInterface(interface_descriptor.second);
	}
	
	// 3. Features
	// TODO: this
	
	// Set default FP state
	// TODO: this
}

MemoryInterface& ThreadInstance::GetMemoryInterface(const std::string& interface_name)
{
	return *memory_interfaces_.at(interface_name);
}


Address RegisterFileInterface::GetTaggedSlot(const std::string &tag) const
{
	auto descriptor = descriptor_.GetTaggedEntry(tag);
	
	switch(descriptor.GetEntrySize()) {
		case 4:
			return Address(*(uint32_t*)(GetData() + descriptor.GetOffset()));
		case 8:
			return Address(*(uint64_t*)(GetData() + descriptor.GetOffset()));
		default:
			UNIMPLEMENTED;
	}
}

void RegisterFileInterface::SetTaggedSlot(const std::string &tag, Address target)
{
	auto descriptor = descriptor_.GetTaggedEntry(tag);
	
	switch(descriptor.GetEntrySize()) {
		case 4:
			*(uint32_t*)(GetData() + descriptor.GetOffset()) = target.Get();
			break;
		case 8:
			*(uint64_t*)(GetData() + descriptor.GetOffset()) = target.Get();
			break;
		default:
			UNIMPLEMENTED;
	}
}


MemoryInterface &ThreadInstance::GetFetchMI()
{
	return *memory_interfaces_.at(descriptor_.GetMemoryInterfaceDescriptor().GetFetchInterface().GetName());
}

archsim::abi::ExceptionAction ThreadInstance::TakeException(uint64_t category, uint64_t data)
{
	return emu_model_.HandleException(this, category, data);
}
