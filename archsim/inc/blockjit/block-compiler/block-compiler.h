/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
#include <wutils/small-set.h>
#include "core/thread/ThreadInstance.h"
#include <wutils/vbitset.h>

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
			class CompileResult
			{
			public:
				CompileResult(bool Success) : Success(Success), UsedPhysRegs(0) {}
				CompileResult(bool Success, uint32_t StackFrameSize, const wutils::vbitset<> &bitset) : Success(Success), StackFrameSize(StackFrameSize), UsedPhysRegs(bitset) {}

				bool Success;
				wutils::vbitset<> UsedPhysRegs;
				uint32_t StackFrameSize;
			};

			class BlockCompiler
			{
			public:
				BlockCompiler(TranslationContext& ctx, uint32_t pa, wulib::MemAllocator &allocator, bool emit_interrupt_check = false, bool emit_chaining_logic = false);
				CompileResult compile(bool dump_intermediates = false);

				bool emit_interrupt_check;
				bool emit_chaining_logic;

				uint32_t GetBlockPA() const
				{
					return pa;
				}
			private:
				wulib::MemAllocator &_allocator;
				TranslationContext& ctx;
				uint32_t pa;

				typedef std::map<shared::IRBlockId, std::vector<shared::IRBlockId>> cfg_t;
				typedef std::vector<shared::IRBlockId> block_list_t;

				bool build_cfg(block_list_t& blocks, cfg_t& succs, cfg_t& preds, block_list_t& exits);
				bool post_allocate_peephole();


			public:



			};
		}
	}
}

#endif	/* BLOCK_COMPILER_H */

