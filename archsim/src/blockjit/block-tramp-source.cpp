/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdint>

#include "gensim/gensim_processor_state.h"
#include "gensim/gensim_processor_blockjit.h"
#include "blockjit/BlockCache.h"


volatile register uint8_t* regstate asm("rbp");
volatile register struct cpuState *cpustate asm("r12");

static void block_trampoline_loop(const struct archsim::blockjit::BlockCacheEntry *block_cache, uint32_t *pc_ptr)
{
	while (!cpustate->pending_actions) {
		uint64_t pc = *(uint32_t*)(pc_ptr);

		uint64_t entry_idx = pc & BLOCKCACHE_MASK;
		entry_idx >>= BLOCKCACHE_INSTRUCTION_SHIFT;

		const auto & entry  = block_cache[entry_idx];

		if(entry.virt_tag == pc) {
			entry.ptr();
			asm volatile ("":::"r15", "r14", "r13", "rbx", "rbp");
		} else {
			break;
		}
	}
}

int block_trampoline_source(struct cpuState *state, struct archsim::blockjit::BlockCacheEntry *block_cache, uint32_t *pc_ptr)
{
	uint64_t backup_rbp, backup_r12;
	asm volatile ("mov %%rbp, %0" : "=m"(backup_rbp));
	asm volatile ("mov %%r12, %0" : "=m"(backup_r12));

	regstate = (uint8_t*)state->gensim_state;
	cpustate = state;

	cpustate->iterations = 10000;

	// IMPORTANT:
	// In this loop, the following registers must not be written by the compiler:
	// rbp, rbx, r12, r13, r14, r15
	block_trampoline_loop(block_cache, pc_ptr);

	asm volatile ("mov %0, %%rbp" :: "m"(backup_rbp));
	asm volatile ("mov %0, %%r12" :: "m"(backup_r12));
	return 0;
}
