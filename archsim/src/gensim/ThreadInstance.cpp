/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/ThreadInstance.h"
#include "gensim/MemoryInterface.h"

using namespace archsim;

ThreadInstance::ThreadInstance(const ArchDescriptor& arch, StateBlockDescriptor &state_block_desc) : descriptor_(arch), state_block_(state_block_desc)
{
	// Need to fill in structures based on arch descriptor info
	
	// 1. Register file
	register_file_.resize(arch.GetRegisterFileDescriptor().GetSize());
	
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


Address ThreadInstance::GetTaggedSlot(const std::string &tag)
{
	auto pc_descriptor = descriptor_.GetRegisterFileDescriptor().GetTaggedEntry(tag);
	
	switch(pc_descriptor.GetEntrySize()) {
		case 4:
			return Address(*(uint32_t*)(register_file_.data() + pc_descriptor.GetOffset()));
		case 8:
			return Address(*(uint64_t*)(register_file_.data() + pc_descriptor.GetOffset()));
		default:
			UNIMPLEMENTED;
	}
}

void ThreadInstance::SetTaggedSlot(const std::string &tag, Address target)
{
	auto pc_descriptor = descriptor_.GetRegisterFileDescriptor().GetTaggedEntry(tag);
	
	switch(pc_descriptor.GetEntrySize()) {
		case 4:
			*(uint32_t*)(register_file_.data() + pc_descriptor.GetOffset()) = target.Get();
			break;
		case 8:
			*(uint64_t*)(register_file_.data() + pc_descriptor.GetOffset()) = target.Get();
			break;
		default:
			UNIMPLEMENTED;
	}
}


MemoryInterface &ThreadInstance::GetFetchMI()
{
	return *memory_interfaces_.at(descriptor_.GetMemoryInterfaceDescriptor().GetFetchInterface().GetName());
}

