/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerTrunc.cpp
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

bool LowerTrunc::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->is_vreg()) {
		if (source->is_alloc_reg() && dest->is_alloc_reg()) {
			// trunc reg -> reg
			if (source->alloc_data != dest->alloc_data) {
				Encoder().mov(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(dest, source->size));
			}
		} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
			// trunc reg -> stack
			Encoder().mov(GetLoweringContext().register_from_operand(source), GetLoweringContext().stack_from_operand(dest));
		} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
			// trunc stack -> reg
			Encoder().mov(GetLoweringContext().stack_from_operand(source), GetLoweringContext().register_from_operand(dest));
		} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
			// trunc stack -> stack
			if (source->alloc_data != dest->alloc_data) {
				Encoder().mov(GetLoweringContext().stack_from_operand(source), GetLoweringContext().get_temp(0, source->size));
				Encoder().mov(GetLoweringContext().get_temp(0, dest->size), GetLoweringContext().stack_from_operand(dest));
			}
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}

	insn++;
	return true;
}
