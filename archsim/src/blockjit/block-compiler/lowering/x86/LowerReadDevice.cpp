/*
 * LowerReadDevice.cpp
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

bool LowerReadDevice::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *dev = &insn->operands[0];
	const IROperand *reg = &insn->operands[1];
	const IROperand *val = &insn->operands[2];

	// Allocate a slot on the stack for the reference argument
	Encoder().sub(8, REG_RSP);
	Encoder().push(0);

	// Load the address of the stack slot into RCX
	Encoder().mov(REG_RSP, REG_RCX);

	GetLoweringContext().emit_save_reg_state(4, GetStackMap(), GetIsStackFixed());

	GetLoweringContext().load_state_field("thread_ptr", REG_RDI);

	GetLoweringContext().encode_operand_function_argument(dev, REG_RSI, GetStackMap());
	GetLoweringContext().encode_operand_function_argument(reg, REG_RDX, GetStackMap());

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&devReadDevice, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	GetLoweringContext().emit_restore_reg_state(GetIsStackFixed());

	// Pop the reference argument value into the destination register
	if (val->is_alloc_reg()) {
		Encoder().pop(GetLoweringContext().register_from_operand(val, 8));
		Encoder().add(8, REG_RSP);
	} else {
		assert(false);
	}

	insn++;
	return true;
}






