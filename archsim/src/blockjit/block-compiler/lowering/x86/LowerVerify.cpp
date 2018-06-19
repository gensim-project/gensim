/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerVerify.cpp
 *
 *  Created on: 17 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

extern "C" void jit_verify(void *cpu);

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerVerify::Lower(const captive::shared::IRInstruction *&insn)
{
	UNIMPLEMENTED;
//	GetCompiler().emit_save_reg_state(insn->count_operands(), GetStackMap(), GetIsStackFixed());
//
//	GetCompiler().load_state_field(0, REG_RDI);
//
//	// Load the address of the target function into a temporary, and perform an indirect call.
//	Encoder().mov((uint64_t)&jit_verify, BLKJIT_RETURN(8));
//	Encoder().call(BLKJIT_RETURN(8));
//
//	GetCompiler().emit_restore_reg_state(insn->count_operands(), GetStackMap(), GetIsStackFixed());
//
//	insn++;
//	return true;
}
