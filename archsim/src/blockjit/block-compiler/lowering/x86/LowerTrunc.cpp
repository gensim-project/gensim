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
#include "blockjit/blockjit-abi.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerTrunc::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->is_vreg()) {
		if (source->is_alloc_reg() && dest->is_alloc_reg()) {
			// trunc reg -> reg
			if (source->alloc_data != dest->alloc_data) {
				Encoder().mov(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest, source->size));
			}
		} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
			// trunc reg -> stack
			Encoder().mov(GetCompiler().register_from_operand(source), GetCompiler().stack_from_operand(dest));
		} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
			// trunc stack -> reg
			Encoder().mov(GetCompiler().stack_from_operand(source), GetCompiler().register_from_operand(dest));
		} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
			// trunc stack -> stack
			if (source->alloc_data != dest->alloc_data) {
				Encoder().mov(GetCompiler().stack_from_operand(source), GetCompiler().get_temp(0, source->size));
				Encoder().mov(GetCompiler().get_temp(0, dest->size), GetCompiler().stack_from_operand(dest));
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
