/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerWriteReg.cpp
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

bool LowerWriteReg::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *offset = &insn->operands[1];
	const IROperand *value = &insn->operands[0];

	if (offset->is_constant()) {
		if (value->is_constant()) {
			switch (value->size) {
				case 8:
					Encoder().mov(value->value, BLKJIT_TEMPS_0(8));
					Encoder().mov(BLKJIT_TEMPS_0(8), X86Memory::get(BLKJIT_REGSTATE_REG, offset->value));
					break;
				case 4:
					Encoder().mov4(value->value, X86Memory::get(BLKJIT_REGSTATE_REG, offset->value));
					break;
				case 2:
					Encoder().mov2(value->value, X86Memory::get(BLKJIT_REGSTATE_REG, offset->value));
					break;
				case 1:
					Encoder().mov1(value->value, X86Memory::get(BLKJIT_REGSTATE_REG, offset->value));
					break;
				default:
					CANTLOWER;
			}
		} else if (value->is_vreg()) {
			if (value->is_alloc_reg()) {
				Encoder().mov(GetLoweringContext().register_from_operand(value), X86Memory::get(BLKJIT_REGSTATE_REG, offset->value));
			} else if (value->is_alloc_stack()) {
				auto& tmp = GetLoweringContext().get_temp(0, value->size);
				Encoder().mov(GetLoweringContext().stack_from_operand(value), tmp);
				Encoder().mov(tmp, X86Memory::get(BLKJIT_REGSTATE_REG, offset->value));
			} else {
				CANTLOWER;
			}
		} else {
			CANTLOWER;
		}
	} else if(offset->is_vreg()) {
		if(value->is_constant()) {
			CANTLOWER;
		} else if(value->is_vreg()) {
			auto &tmp_value = GetLoweringContext().get_temp(0, value->size);
			auto &tmp_offset = GetLoweringContext().get_temp(1, offset->size);
			if(offset->is_alloc_reg()) {
				Encoder().mov(GetLoweringContext().register_from_operand(offset), tmp_offset);
			} else {
				CANTLOWER;
			}

			Encoder().mov(tmp_value, X86Memory::get(BLKJIT_REGSTATE_REG, tmp_offset, 1));

		} else {
			CANTLOWER;
		}
	} else {
		CANTLOWER;
	}
	insn++;
	return true;
}
