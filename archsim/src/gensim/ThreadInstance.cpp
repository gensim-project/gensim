/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/ThreadInstance.h"

using namespace archsim;

ThreadInstance::ThreadInstance(const ArchDescriptor& arch, StateBlockDescriptor &state_block_desc) : descriptor_(arch), state_block_(state_block_desc)
{
	// Need to fill in structures based on arch descriptor info
	
	// 1. Register file
	register_file_.resize(arch.GetRegisterFileDescriptor().GetSize());
	
	// 2. Memory interfaces
	for(auto interface_descriptor : arch.GetMemoryInterfaceDescriptor().GetInterfaces()) {
		memory_interfaces_[interface_descriptor.second.GetName()] = new MemoryInterface(interface_descriptor.second);
	}
	
	// 3. Features
	// TODO: this
	
	// Set default FP state
	// TODO: this
}
