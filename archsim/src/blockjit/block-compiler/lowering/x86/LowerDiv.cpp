/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerAdcFlags.cpp
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

bool LowerUDiv::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	auto &temp = GetLoweringContext().get_temp(0, 4);
	// back up EAX
	Encoder().mov(REG_RAX, BLKJIT_TEMPS_1(8));

	Encoder().xorr(REG_EDX, REG_EDX);

	GetLoweringContext().encode_operand_to_reg(dest, REG_EAX);
	GetLoweringContext().encode_operand_to_reg(source, temp);

	Encoder().div(temp);
	Encoder().mov(REG_EAX, temp);

	Encoder().mov(BLKJIT_TEMPS_1(8), REG_RAX);

	if (dest->is_alloc_reg()) {
		Encoder().mov(temp, GetLoweringContext().register_from_operand(dest, 4));
	} else {
		assert(false);
	}

	insn++;
	return true;
}

bool LowerSDiv::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	auto &temp = GetLoweringContext().get_temp(0, 4);
	auto &dest_reg = GetLoweringContext().register_from_operand(dest);
	// back up EAX
	
	if(dest_reg != REGS_RAX(dest->size)) {
		Encoder().mov(REG_RAX, BLKJIT_TEMPS_1(8));
	}

	GetLoweringContext().encode_operand_to_reg(source, temp);
	GetLoweringContext().encode_operand_to_reg(dest, REG_EAX);
	

	Encoder().cltd();
	Encoder().idiv(temp);
	Encoder().mov(REG_EAX, temp);

	if(dest_reg != REGS_RAX(dest->size)) {
		Encoder().mov(BLKJIT_TEMPS_1(8), REG_RAX);
	}

	if (dest->is_alloc_reg()) {
		Encoder().mov(temp, dest_reg);
	} else {
		assert(false);
	}

	insn++;
	return true;
}
