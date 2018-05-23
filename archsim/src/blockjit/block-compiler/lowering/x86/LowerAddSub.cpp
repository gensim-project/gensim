/*
 * LowerAddSub.cpp
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

bool LowerAddSub::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->type == IROperand::VREG) {
		if (source->is_alloc_reg() && dest->is_alloc_reg()) {
			switch (insn->type) {
				case IRInstruction::ADD:
					Encoder().add(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(dest));
					break;
				case IRInstruction::SUB:
					Encoder().sub(GetLoweringContext().register_from_operand(source), GetLoweringContext().register_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
			switch (insn->type) {
				case IRInstruction::ADD:
					Encoder().add(GetLoweringContext().register_from_operand(source), GetLoweringContext().stack_from_operand(dest));
					break;
				case IRInstruction::SUB:
					Encoder().sub(GetLoweringContext().register_from_operand(source), GetLoweringContext().stack_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
			switch (insn->type) {
				case IRInstruction::ADD:
					Encoder().add(GetLoweringContext().stack_from_operand(source), GetLoweringContext().register_from_operand(dest));
					break;
				case IRInstruction::SUB:
					Encoder().sub(GetLoweringContext().stack_from_operand(source), GetLoweringContext().register_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
			Encoder().mov(GetLoweringContext().stack_from_operand(source), BLKJIT_TEMPS_0(source->size));
			switch(insn->type) {
				case IRInstruction::ADD:
					Encoder().add(BLKJIT_TEMPS_0(source->size), GetLoweringContext().stack_from_operand(dest));
					break;
				case IRInstruction::SUB:
					Encoder().sub(BLKJIT_TEMPS_0(source->size), GetLoweringContext().stack_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else {
			assert(false);
		}
	} else if (source->type == IROperand::PC) {
		if (dest->is_alloc_reg()) {
			// TODO: FIXME: HACK HACK HACK XXX
			switch (insn->type) {
				case IRInstruction::ADD:
					Encoder().add(X86Memory::get(BLKJIT_REGSTATE_REG, 60), GetLoweringContext().register_from_operand(dest));
					break;
				case IRInstruction::SUB:
					Encoder().sub(X86Memory::get(BLKJIT_REGSTATE_REG, 60), GetLoweringContext().register_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else {
			assert(false);
		}
	} else if (source->type == IROperand::CONSTANT) {
		if (dest->is_alloc_reg()) {
			if (source->size == 8) {
				auto& tmp = GetLoweringContext().get_temp(0, dest->size);

				Encoder().mov(source->value, tmp);

				switch (insn->type) {
					case IRInstruction::ADD:
						Encoder().add(tmp, GetLoweringContext().register_from_operand(dest));
						break;
					case IRInstruction::SUB:
						Encoder().sub(tmp, GetLoweringContext().register_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			} else {
				switch (insn->type) {
					case IRInstruction::ADD:
						Encoder().add(source->value, GetLoweringContext().register_from_operand(dest));
						break;
					case IRInstruction::SUB:
						Encoder().sub(source->value, GetLoweringContext().register_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			}
		} else if(dest->is_alloc_stack()) {
			switch(insn->type) {
				case IRInstruction::ADD:
					Encoder().add(source->value, dest->size, GetLoweringContext().stack_from_operand(dest));
					break;
				case IRInstruction::SUB:
					Encoder().sub(source->value, dest->size, GetLoweringContext().stack_from_operand(dest));
					break;
				default:
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

