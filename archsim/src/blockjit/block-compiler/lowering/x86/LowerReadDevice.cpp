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
#include "blockjit/blockjit-abi.h"

#include "translate/jit_funs.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
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

	GetCompiler().emit_save_reg_state(4, GetStackMap(), GetIsStackFixed());

	GetCompiler().load_state_field(0, REG_RDI);

	GetCompiler().encode_operand_function_argument(dev, REG_RSI, GetStackMap());
	GetCompiler().encode_operand_function_argument(reg, REG_RDX, GetStackMap());

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&devReadDevice, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(4, GetStackMap(), GetIsStackFixed());

	// Pop the reference argument value into the destination register
	if (val->is_alloc_reg()) {
		Encoder().pop(GetCompiler().register_from_operand(val, 8));
		Encoder().add(8, REG_RSP);
	} else {
		assert(false);
	}

	insn++;
	return true;
}






