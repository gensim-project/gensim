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

bool LowerProbeDevice::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *dev = &insn->operands[0];
	const IROperand *reg = &insn->operands[1];


	// Load the address of the stack slot into RCX
	Encoder().mov(REG_RSP, REG_RCX);

	GetLoweringContext().emit_save_reg_state(4, GetStackMap(), GetIsStackFixed());

	GetLoweringContext().load_state_field(0, REG_RDI);

	GetLoweringContext().encode_operand_function_argument(dev, REG_RSI, GetStackMap());

	// Load the address of the target function into a temporary, and perform an indirect call.
	Encoder().mov((uint64_t)&devProbeDevice, BLKJIT_RETURN(8));
	Encoder().call(BLKJIT_RETURN(8));

	Encoder().push(REG_RAX);

	GetLoweringContext().emit_restore_reg_state(GetIsStackFixed());

	// Pop the reference argument value into the destination register
	if (reg->is_alloc_reg()) {
		Encoder().pop(GetLoweringContext().register_from_operand(reg, 8));
	} else {
		assert(false);
	}

	insn++;
	return true;
}






