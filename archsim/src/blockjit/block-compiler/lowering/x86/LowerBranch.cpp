/*
 * LowerBranch.cpp
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

bool LowerBranch::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *cond = &insn->operands[0];
	const IROperand *tt = &insn->operands[1];
	const IROperand *ft = &insn->operands[2];

	if (cond->is_vreg()) {
		if (cond->is_alloc_reg()) {
			Encoder().test(GetLoweringContext().register_from_operand(cond), GetLoweringContext().register_from_operand(cond));
		} else {
			auto& tmp = GetLoweringContext().get_temp(0, 1);

			Encoder().mov(GetLoweringContext().stack_from_operand(cond), tmp);
			Encoder().test(tmp, tmp);
		}
	} else {
		if(cond->is_constant()) {
			IRBlockId target = cond->value ? tt->value : ft->value;
			uint32_t reloc_offset;
			Encoder().jmp_reloc(reloc_offset);
			GetLoweringContext().RegisterBlockRelocation(reloc_offset, target);

			Encoder().align_up(8);
			insn++;
			return true;
		}
		assert(false);
	}

	const IRInstruction *next_insn = insn+1;

	if (next_insn && next_insn->ir_block == (IRBlockId)tt->value) {
		// Fallthrough is TRUE block
		{
			uint32_t reloc_offset;
			Encoder().jz_reloc(reloc_offset);
			GetLoweringContext().RegisterBlockRelocation(reloc_offset, ft->value);
		}
	} else if (next_insn && next_insn->ir_block == (IRBlockId)ft->value) {
		// Fallthrough is FALSE block
		{
			uint32_t reloc_offset;
			Encoder().jnz_reloc(reloc_offset);
			GetLoweringContext().RegisterBlockRelocation(reloc_offset, tt->value);
		}
	} else {
		// Fallthrough is NEITHER
		{
			uint32_t reloc_offset;
			Encoder().jnz_reloc(reloc_offset);
			GetLoweringContext().RegisterBlockRelocation(reloc_offset, tt->value);
		}

		{
			uint32_t reloc_offset;
			Encoder().jmp_reloc(reloc_offset);
			GetLoweringContext().RegisterBlockRelocation(reloc_offset, ft->value);
		}

		Encoder().align_up(8);
	}


	insn++;
	return true;
}






