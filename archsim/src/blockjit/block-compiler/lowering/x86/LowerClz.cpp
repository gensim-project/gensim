/*
 * LowerClz.cpp
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

bool LowerClz::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->is_vreg()) {
		if (dest->is_vreg()) {
			if (source->is_alloc_reg() && dest->is_alloc_reg()) {
				Encoder().bsr(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
				Encoder().xorr(0x1f, GetCompiler().register_from_operand(dest));
			} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
				assert(false);
			} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
				assert(false);
			} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
				assert(false);
			} else {
				assert(false);
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


