/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdint>

#include "core/thread/ThreadInstance.h"
#include "blockjit/BlockCache.h"

template <typename PC_t> static void block_trampoline_loop(archsim::core::thread::ThreadInstance *thread, const struct archsim::blockjit::BlockCacheEntry *block_cache, PC_t *pc_ptr)
{
	void *regfile = thread->GetRegisterFile();
	while (!thread->HasMessage()) {
		uint64_t pc = *(PC_t*)(pc_ptr);

		uint64_t entry_idx = pc & BLOCKCACHE_MASK;
		entry_idx >>= BLOCKCACHE_INSTRUCTION_SHIFT;

		const auto & entry  = block_cache[entry_idx];

		if(entry.virt_tag == pc) {
			entry.ptr(regfile, thread->GetStateBlock().GetData());
			asm volatile ("":::"r15", "r14", "r13", "rbx", "rbp");
		} else {
			break;
		}
	}
}

int block_trampoline_source(archsim::core::thread::ThreadInstance *thread, const archsim::RegisterFileEntryDescriptor &pc_descriptor, struct archsim::blockjit::BlockCacheEntry *block_cache)
{
	uint64_t backup_rbp, backup_r12;
	
	// IMPORTANT:
	// In this loop, the following registers must not be written by the compiler:
	// rbp, rbx, r12, r13, r14, r15
	
	switch(pc_descriptor.GetEntrySize()) {
		case 4:
			block_trampoline_loop<uint32_t>(thread, block_cache, (uint32_t*)((uint8_t*)thread->GetRegisterFile() + pc_descriptor.GetOffset()));
			break;
		case 8:
			block_trampoline_loop<uint64_t>(thread, block_cache, (uint64_t*)((uint8_t*)thread->GetRegisterFile() + pc_descriptor.GetOffset()));
			break;
		default:
			UNIMPLEMENTED;
	}
	
	return 0;
}
