/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerIntCheck::Lower(const captive::shared::IRInstruction *&insn)
{
	UNIMPLEMENTED;
	
//	// load interrupt check field
//	GetCompiler().load_state_field(gensim::CpuStateOffsets::CpuState_pending_actions, BLKJIT_TEMPS_0(4));
//
//	// if field is zero, skip ret instruction
//	Encoder().test(BLKJIT_TEMPS_0(4), BLKJIT_TEMPS_0(4));
//
//	uint32_t dontreturn;
//	Encoder().je_short_reloc(dontreturn);
//
//	uint32_t max_stack = GetLoweringContext().GetStackFrameSize();
//	if(max_stack & 15) {
//		max_stack = (max_stack & ~15) + 16;
//	}
//	if(max_stack)
//		Encoder().add(max_stack, REG_RSP);
//
//	Encoder().ret();
//
//	*(uint8_t*)(Encoder().get_buffer() + dontreturn) = Encoder().current_offset() - dontreturn - 1;

	insn++;
	return true;
}
