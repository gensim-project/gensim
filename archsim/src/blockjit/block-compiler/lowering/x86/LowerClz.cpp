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
			
			auto source_reg = &BLKJIT_TEMPS_0(source->size);
			auto dest_reg = &BLKJIT_TEMPS_1(dest->size);
			
			if(source->is_alloc_reg()) {
				source_reg = &GetCompiler().register_from_operand(source);
			} else {
				GetCompiler().encode_operand_to_reg(source, *source_reg);
			}
			if(dest->is_alloc_reg()) {
				dest_reg = &GetCompiler().register_from_operand(dest);
			} else {
				GetCompiler().encode_operand_to_reg(dest, *dest_reg);
			}
			
			Encoder().bsr(*source_reg, *dest_reg);
			Encoder().xorr(0x1f, *dest_reg);
			
			if(dest->is_alloc_stack()) {
				Encoder().mov(*dest_reg, GetCompiler().stack_from_operand(dest));
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


