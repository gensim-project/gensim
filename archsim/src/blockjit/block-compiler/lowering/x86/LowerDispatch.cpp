/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerDispatch.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerDispatch::Lower(const captive::shared::IRInstruction *&insn)
{
	UNIMPLEMENTED;
//	const auto *cpu = GetCompiler().get_cpu();
//	uint32_t pc_offset = cpu->GetRegisterByTag("PC")->Offset;
//
//	// If we shouldn't dispatch, just do a return instead
//	if(!GetCompiler().emit_chaining_logic) {
//		LowerRet r;
//		r.SetContexts(GetLoweringContext(),GetTranslationContext(), GetCompiler());
//		r.Lower(insn);
//		// insn incremented by return lowering
//		return true;
//	}
//
//	// Dispatch instruction can have three general forms:
//	// 1. All operands populated - instruction is a predicated jump and both target and fallthrough are compiled
//	// 2. Fallthrough function not populated - instruction is a predicated jump and only the target is compiled
//	// 3. Fallthrough PC and function not populated - instruction is a non predicated jump
//
//	const IROperand &target_pc = insn->operands[0];
//	const IROperand &fallthrough_pc = insn->operands[1];
//	const IROperand &target_fn = insn->operands[2];
//	const IROperand &fallthrough_fn = insn->operands[3];
//
//	assert(target_fn.value != 0 || fallthrough_fn.value != 0);
//
//	// We need to do some stuff no matter what kind of dispatch we're doing
//	// 1. fix up stack
//	// 2. make sure we don't realloc halfway through emitting dispatch code and wreck the offsets
//	// 3. do an interrupt check
//
//	if(GetLoweringContext().GetStackFrameSize()) {
//		Encoder().add(GetLoweringContext().GetStackFrameSize(), REG_RSP);
//	}
//
////	Encoder().int3();
//
//	// Decrement iteration count
//	Encoder().cmp4(0, X86Memory::get(BLKJIT_CPUSTATE_REG, gensim::CpuStateOffsets::CpuState_pending_actions));
//
//	Encoder().ensure_extra_buffer(32);
//
//	// If iteration count is nonzero...
//	uint32_t iteration_count_zero;
//	Encoder().jne_reloc(iteration_count_zero);
//
//	uint32_t pc_not_target = 0;
//	uint32_t fallthrough_not_target = 0;
//	if(target_fn.value != 0) {
//		// compare PC against target PC...
//		Encoder().cmp4((uint32_t)target_pc.value, X86Memory::get(REG_RBP, pc_offset)); // XXX ARM HACK
//		Encoder().jne_reloc(pc_not_target);
//		if(archsim::options::Verbose.GetValue()) {
//			Encoder().add(1, 8, X86Memory::get(BLKJIT_CPUSTATE_REG, gensim::CpuStateOffsets::CpuState_chained_target));
//		}
//		Encoder().jmp((void*)target_fn.value, REG_R15);
//	}
//	if(fallthrough_fn.value != 0) {
//		// compare PC against target PC...
//		Encoder().cmp4((uint32_t)fallthrough_pc.value, X86Memory::get(REG_RBP, pc_offset)); // XXX ARM HACK
//		Encoder().jne_reloc(fallthrough_not_target);
//		if(archsim::options::Verbose.GetValue()) {
//			Encoder().add(1, 8, X86Memory::get(BLKJIT_CPUSTATE_REG, gensim::CpuStateOffsets::CpuState_chained_fallthrough));
//		}
//		Encoder().jmp((void*)fallthrough_fn.value, REG_R15);
//	}
//
//
//	// fill in jump targets
//	*(uint32_t*)(Encoder().get_buffer() + iteration_count_zero) = Encoder().current_offset() - iteration_count_zero - 4;
//	if(fallthrough_not_target != 0) {
//		*(uint32_t*)(Encoder().get_buffer() + fallthrough_not_target) = Encoder().current_offset() - fallthrough_not_target - 4;
//	}
//	if(pc_not_target != 0) {
//		*(uint32_t*)(Encoder().get_buffer() + pc_not_target) = Encoder().current_offset() - pc_not_target - 4;
//	}
//
//	if(archsim::options::Verbose.GetValue()) {
//		Encoder().add(1, 8, X86Memory::get(BLKJIT_CPUSTATE_REG, gensim::CpuStateOffsets::CpuState_chained_failed));
//	}
//
//	Encoder().xorr(REG_RAX, REG_RAX);
//	Encoder().ret();
//
//	insn++;
//	return true;
}
