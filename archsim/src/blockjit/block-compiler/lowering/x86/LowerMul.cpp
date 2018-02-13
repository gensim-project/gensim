/*
 * LowerMul.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerMul::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	auto &dest_reg = dest->is_alloc_reg() ? GetCompiler().register_from_operand(dest) : BLKJIT_TEMPS_1(source->size);

	if (dest->is_vreg()) {
		if (source->is_alloc_reg() && dest->is_alloc_reg()) {
			Encoder().mul(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
		} else if(source->is_alloc_stack() && dest->is_alloc_reg()) {//0x7ffff7e34f70:
			Encoder().mov(GetCompiler().stack_from_operand(source), BLKJIT_TEMPS_0(source->size));
			Encoder().mul(BLKJIT_TEMPS_0(source->size), dest_reg);
		} else if(source->is_constant() && dest->is_alloc_reg()) {
			Encoder().mov(source->value, BLKJIT_TEMPS_0(source->size));
			Encoder().mul(BLKJIT_TEMPS_0(source->size), dest_reg);
		}
	} else {
		assert(false);
	}

	if(dest->is_alloc_stack()) {
		Encoder().mov(BLKJIT_TEMPS_1(source->size), GetCompiler().stack_from_operand(dest));
	}

	insn++;
	return true;
}
