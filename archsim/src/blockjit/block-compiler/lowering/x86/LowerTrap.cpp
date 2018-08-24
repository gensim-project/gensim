/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerTrap.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"

using namespace captive::arch::jit::lowering::x86;

bool LowerTrap::Lower(const captive::shared::IRInstruction *&insn)
{
	Encoder().int3();
	insn++;
	return true;
}

