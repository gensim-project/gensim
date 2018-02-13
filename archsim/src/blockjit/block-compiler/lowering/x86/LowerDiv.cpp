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
#include "blockjit/blockjit-abi.h"
#include "gensim/gensim_processor_blockjit.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerUDiv::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	auto &temp = GetCompiler().get_temp(0, 4);
	// back up EAX
	Encoder().mov(REG_RAX, BLKJIT_TEMPS_1(8));

	Encoder().xorr(REG_EDX, REG_EDX);

	GetCompiler().encode_operand_to_reg(dest, REG_EAX);
	GetCompiler().encode_operand_to_reg(source, temp);

	Encoder().div(temp);
	Encoder().mov(REG_EAX, temp);

	Encoder().mov(BLKJIT_TEMPS_1(8), REG_RAX);

	if (dest->is_alloc_reg()) {
		Encoder().mov(temp, GetCompiler().register_from_operand(dest, 4));
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

	auto &temp = GetCompiler().get_temp(0, 4);
	// back up EAX
	Encoder().mov(REG_RAX, BLKJIT_TEMPS_1(8));

	GetCompiler().encode_operand_to_reg(dest, REG_EAX);
	GetCompiler().encode_operand_to_reg(source, temp);

	Encoder().cltd();
	Encoder().idiv(temp);
	Encoder().mov(REG_EAX, temp);

	Encoder().mov(BLKJIT_TEMPS_1(8), REG_RAX);

	if (dest->is_alloc_reg()) {
		Encoder().mov(temp, GetCompiler().register_from_operand(dest, 4));
	} else {
		assert(false);
	}

	insn++;
	return true;
}