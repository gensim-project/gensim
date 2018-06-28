/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerCount.cpp
 *
 *  Created on: 30 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerCount::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *counter = &insn->operands[0];
	const IROperand *amount = &insn->operands[1];

	Encoder().mov(counter->value, BLKJIT_ARG0(8));
	Encoder().add8(amount->value, X86Memory::get(BLKJIT_ARG0(8)));

	insn++;
	return true;
}

