/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerTakeException.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "translate/jit_funs.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerTakeException::Lower(const captive::shared::IRInstruction *&insn)
{
	GetLoweringContext().emit_save_reg_state(3, GetStackMap(), GetIsStackFixed());

	GetLoweringContext().load_state_field("thread_ptr", REG_RDI);
	GetLoweringContext().encode_operand_function_argument(&insn->operands[0], REG_RSI, GetStackMap());
	GetLoweringContext().encode_operand_function_argument(&insn->operands[1], REG_RDX, GetStackMap());

	Encoder().mov((uint64_t)(cpuTakeException), BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	GetLoweringContext().emit_restore_reg_state(GetIsStackFixed());

	insn++;
	return true;
}


