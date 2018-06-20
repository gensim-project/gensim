/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerExtend.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerExtend::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->is_vreg()) {

		if (source->size == 4 && dest->size == 8) {
			if (insn->type == IRInstruction::ZX) {
				if (source->is_alloc_reg() && dest->is_alloc_reg()) {
					Encoder().mov(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(dest, 4));
				} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
					Encoder().mov(GetLoweringContext().stack_from_operand(source), GetLoweringContext().register_from_operand(dest, 4));
				} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
					Encoder().mov(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(source));
					Encoder().mov(GetLoweringContext().register_from_operand(source, 8), GetLoweringContext().stack_from_operand(dest));
				} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
					Encoder().mov(GetLoweringContext().stack_from_operand(source), BLKJIT_TEMPS_0(4));
					Encoder().mov(BLKJIT_TEMPS_0(8), GetLoweringContext().stack_from_operand(dest));
				} else {
					assert(false);
				}
			} else {
				auto source_reg = source->is_alloc_reg() ? GetLoweringContext().register_from_operand(source) : BLKJIT_TEMPS_0(source->size);
				auto dest_reg = dest->is_alloc_reg() ? GetLoweringContext().register_from_operand(dest) : BLKJIT_TEMPS_1(dest->size);

				if(source->is_alloc_stack()) {
					Encoder().mov(GetLoweringContext().stack_from_operand(source), source_reg);
				}

				Encoder().movsx(source_reg, dest_reg);

				if(dest->is_alloc_stack()) {
					Encoder().mov(dest_reg, GetLoweringContext().stack_from_operand(dest));
				}
			}
		} else {
			if (source->is_alloc_reg() && dest->is_alloc_reg()) {
				switch (insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(dest));
						break;

					case IRInstruction::SX:
						if (GetLoweringContext().register_from_operand(source) == REG_EAX && GetLoweringContext().register_from_operand(dest) == REG_RAX) {
							Encoder().cltq();
						} else if (GetLoweringContext().register_from_operand(source) == REG_AX && GetLoweringContext().register_from_operand(dest) == REG_EAX) {
							Encoder().cwtl();
						} else if (GetLoweringContext().register_from_operand(source) == REG_AL && GetLoweringContext().register_from_operand(dest) == REG_AX) {
							Encoder().cbtw();
						} else {
							Encoder().movsx(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(dest));
						}

						break;
					default:
						assert(false);
						break;
				}
			} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
				auto tmpreg = GetLoweringContext().get_temp(0, dest->size);

				switch(insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(GetLoweringContext().register_from_operand(source), tmpreg);
						break;
					case IRInstruction::SX:
						Encoder().movsx(GetLoweringContext().register_from_operand(source), tmpreg);
						break;
					default:
						assert(false);
						break;
				}
				Encoder().mov(tmpreg, GetLoweringContext().stack_from_operand(dest));

			} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
				auto& tmpreg = GetLoweringContext().unspill_temp(source, 0);

				switch (insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(tmpreg, GetLoweringContext().register_from_operand(dest));
						break;

					case IRInstruction::SX:
						Encoder().movsx(tmpreg, GetLoweringContext().register_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
				auto& tmpreg = GetLoweringContext().unspill_temp(source, 0);

				switch (insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(tmpreg, GetLoweringContext().get_temp(0, dest->size));
						Encoder().mov(GetLoweringContext().get_temp(0, dest->size), GetLoweringContext().stack_from_operand(dest));
						break;

					case IRInstruction::SX:
						Encoder().movsx(tmpreg, GetLoweringContext().get_temp(0, dest->size));
						Encoder().mov(GetLoweringContext().get_temp(0, dest->size), GetLoweringContext().stack_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			} else {
				assert(false);
			}
		}
	} else if(source->is_constant()) {
//		LC_WARNING(LogBlockJit) << "Constant extended into register in block " << std::hex << GetLoweringContext().GetBlockPA();
		Encoder().mov(source->value, GetLoweringContext().register_from_operand(dest));
	} else {
		assert(false);
	}

	insn++;
	return true;
}


