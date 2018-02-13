/*
 * LowerTrap.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerCMov::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *predicate = &insn->operands[0];
	const IROperand *yes = &insn->operands[1];
	const IROperand *no = &insn->operands[2];

	// yes might have ended up as a constant
	assert(predicate->is_alloc_reg() && no->is_alloc_reg());
	assert(yes->is_constant() || yes->is_alloc_reg());

	auto *yes_reg = &BLKJIT_TEMPS_0(yes->size);
	if(yes->is_constant()) {
		Encoder().mov(yes->value, BLKJIT_TEMPS_0(yes->size));
	} else {
		yes_reg = &GetCompiler().register_from_operand(yes);
	}

	Encoder().test(GetCompiler().register_from_operand(predicate), GetCompiler().register_from_operand(predicate));
	Encoder().cmovne(*yes_reg, GetCompiler().register_from_operand(no));

	insn++;
	return true;
}

