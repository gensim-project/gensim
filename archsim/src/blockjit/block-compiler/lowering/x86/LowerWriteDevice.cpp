/*
 * LowerWriteDevice.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

#include "translate/jit_funs.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerWriteDevice::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *dev = &insn->operands[0];
	const IROperand *reg = &insn->operands[1];
	const IROperand *val = &insn->operands[2];

	const IRInstruction *prev_insn = insn-1;
	const IRInstruction *next_insn = insn+1;

	if (prev_insn && prev_insn->type == IRInstruction::WRITE_DEVICE) {
		//
	} else {
		GetStackMap().clear();
		GetCompiler().emit_save_reg_state(4, GetStackMap(), GetIsStackFixed());
	}

	GetCompiler().load_state_field("thread_ptr", REG_RDI);

	GetCompiler().encode_operand_function_argument(dev, REG_RSI, GetStackMap());
	GetCompiler().encode_operand_function_argument(reg, REG_RDX, GetStackMap());
	GetCompiler().encode_operand_function_argument(val, REG_RCX, GetStackMap());

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&devWriteDevice, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	if (next_insn && next_insn->type == IRInstruction::WRITE_DEVICE) {
		//
	} else {
		GetCompiler().emit_restore_reg_state(4, GetStackMap(), GetIsStackFixed());
	}

	insn++;
	return true;
}
