/*
 * File:   block-compiler.h
 * Author: spink
 *
 * Created on 27 April 2015, 11:58
 */

#ifndef BLOCK_COMPILER_H
#define	BLOCK_COMPILER_H

#include <define.h>
#include "blockjit/ir.h"
#include "blockjit/block-compiler/lowering/x86/X86Encoder.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"
#include "blockjit/IRInstruction.h"
#include "blockjit/IROperand.h"
#include "blockjit/block-compiler/analyses/Analysis.h"

#include "blockjit/translation-context.h"
#include "util/wutils/small-set.h"
#include "core/thread/ThreadInstance.h"
#include "util/vbitset.h"

#include <map>
#include <vector>
#include <sstream>
#include <iomanip>


namespace captive
{
	namespace arch
	{
		namespace jit
		{
			class BlockCompiler
			{
			public:
				BlockCompiler(TranslationContext& ctx, uint32_t pa, wulib::MemAllocator &allocator, bool emit_interrupt_check = false, bool emit_chaining_logic = false);
				size_t compile(shared::block_txln_fn& fn, bool dump_intermediates = false);

				void dump_ir(std::ostringstream &ostr);
				void dump_ir();

				bool emit_interrupt_check;
				bool emit_chaining_logic;

				void set_cpu(archsim::core::thread::ThreadInstance *cpu)
				{
					_cpu = cpu;
				}
				const archsim::core::thread::ThreadInstance *get_cpu()
				{
					return _cpu;
				}

				uint32_t GetBlockPA() const
				{
					return pa;
				}
			private:
				wulib::MemAllocator &_allocator;
				TranslationContext& ctx;
				captive::arch::jit::lowering::x86::X86Encoder encoder;
				archsim::core::thread::ThreadInstance *_cpu;
				uint32_t pa;


//				PopulatedSet<BLKJIT_NUM_ALLOCABLE> used_phys_regs;

				typedef std::map<shared::IRBlockId, std::vector<shared::IRBlockId>> cfg_t;
				typedef std::vector<shared::IRBlockId> block_list_t;

				bool analyse(uint32_t& max_stack);
				bool build_cfg(block_list_t& blocks, cfg_t& succs, cfg_t& preds, block_list_t& exits);
				bool post_allocate_peephole();
				bool lower(uint32_t max_stack, analyses::HostRegLivenessData &host_liveness);
				bool lower_stack_to_reg(archsim::util::vbitset &used_phys_regs);


			public:



			};
		}
	}
}

#endif	/* BLOCK_COMPILER_H */

