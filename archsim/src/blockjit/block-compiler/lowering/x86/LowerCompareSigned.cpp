/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerCompare.cpp
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

// This class lowers compare instructions, and fuses compare-and-branch pairs where possible.

bool LowerCompareSigned::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *lhs = &insn->operands[0];
	const IROperand *rhs = &insn->operands[1];
	const IROperand *dest = &insn->operands[2];

	// This should be true if the OPERANDS are reversed, not if the CONDITION
	// should be inverted. 
	// This is important since:
	// LH >= RH
	// is the same as 
	// RH <= LH
	
	// and
	
	// LH > RH
	// is the same as 
	// RH < LH
	bool reverse_operands = false;

	switch (lhs->type) {
		case IROperand::VREG: {
			switch (rhs->type) {
				case IROperand::VREG: {
					if (lhs->is_alloc_reg() && rhs->is_alloc_reg()) {
						Encoder().cmp(GetLoweringContext().register_from_operand(rhs), GetLoweringContext().register_from_operand(lhs));
					} else if (lhs->is_alloc_stack() && rhs->is_alloc_reg()) {
						Encoder().cmp(GetLoweringContext().register_from_operand(rhs), GetLoweringContext().stack_from_operand(lhs));
					} else if (lhs->is_alloc_reg() && rhs->is_alloc_stack()) {
						// Apparently we can't yet encode cmp (stack), (reg) so encode cmp (reg), (stack) instead
						// and invert the result
						reverse_operands = true;

						Encoder().cmp(GetLoweringContext().register_from_operand(lhs), GetLoweringContext().stack_from_operand(rhs));
					} else if (lhs->is_alloc_stack() && rhs->is_alloc_stack()) {
						// Apparently we can't yet encode cmp (stack), (reg) so encode cmp (reg), (stack) instead
						// and invert the result
						reverse_operands = true;

						auto& tmp = GetLoweringContext().unspill_temp(lhs, 0);
						Encoder().cmp(tmp, GetLoweringContext().stack_from_operand(rhs));
					} else {
						assert(false);
					}

					break;
				}

				case IROperand::CONSTANT: {
					if (lhs->is_alloc_reg()) {
						Encoder().cmp(rhs->value, GetLoweringContext().register_from_operand(lhs));
					} else if (lhs->is_alloc_stack()) {
						switch (rhs->size) {
							case 1:
								Encoder().cmp1(rhs->value, GetLoweringContext().stack_from_operand(lhs));
								break;
							case 2:
								Encoder().cmp2(rhs->value, GetLoweringContext().stack_from_operand(lhs));
								break;
							case 4:
								Encoder().cmp4(rhs->value, GetLoweringContext().stack_from_operand(lhs));
								break;
							case 8:
								Encoder().cmp8(rhs->value, GetLoweringContext().stack_from_operand(lhs));
								break;
							default:
								assert(false);
						}
					} else {
						assert(false);
					}

					break;
				}

				default:
					assert(false);
			}

			break;
		}

		case IROperand::CONSTANT: {
			reverse_operands = true;

			switch (rhs->type) {
				case IROperand::VREG: {
					if (rhs->is_alloc_reg()) {
						Encoder().cmp(lhs->value, GetLoweringContext().register_from_operand(rhs));
					} else if (rhs->is_alloc_stack()) {
						switch (lhs->size) {
							case 1:
								Encoder().cmp1(lhs->value, GetLoweringContext().stack_from_operand(rhs));
								break;
							case 2:
								Encoder().cmp2(lhs->value, GetLoweringContext().stack_from_operand(rhs));
								break;
							case 4:
								Encoder().cmp4(lhs->value, GetLoweringContext().stack_from_operand(rhs));
								break;
							case 8:
								Encoder().cmp8(lhs->value, GetLoweringContext().stack_from_operand(rhs));
								break;
						}
					} else {
						assert(false);
					}

					break;
				}
				case IROperand::CONSTANT: {
//					LC_WARNING(LogBlockJit) << "Constant compared against constant in block " << std::hex << GetLoweringContext().GetBlockPA();
					Encoder().mov(rhs->value, GetLoweringContext().get_temp(0, rhs->size));
					Encoder().cmp(lhs->value, GetLoweringContext().get_temp(0, rhs->size));
					break;
				}
				default:
					assert(false);
			}

			break;
		}

		default:
			assert(false);
	}

	bool should_branch = false;

	const IRInstruction *next_insn = insn+1;

	// TODO: Look at this optimisation
	if (next_insn && next_insn->type == IRInstruction::BRANCH) {
		// If the next instruction is a branch, we need to check to see if it's branching on
		// this condition, before we go ahead an emit an optimised form.

		if (next_insn->operands[0].is_vreg() && next_insn->operands[0].value == dest->value) {
			// Right here we go, we've got a compare-and-branch situation.

			// Set the do-a-branch-instead flag
			should_branch = true;
		}
	}

	switch (insn->type) {
		case IRInstruction::CMPSLT:
			if (reverse_operands)
				Encoder().setg(GetLoweringContext().register_from_operand(dest));
			else
				Encoder().setl(GetLoweringContext().register_from_operand(dest));
			break;

		case IRInstruction::CMPSLTE:
			if (reverse_operands)
				Encoder().setge(GetLoweringContext().register_from_operand(dest));
			else
				Encoder().setle(GetLoweringContext().register_from_operand(dest));
			break;

		case IRInstruction::CMPSGT:
			if (reverse_operands)
				Encoder().setl(GetLoweringContext().register_from_operand(dest));
			else
				Encoder().setg(GetLoweringContext().register_from_operand(dest));
			break;

		case IRInstruction::CMPSGTE:
			if (reverse_operands)
				Encoder().setle(GetLoweringContext().register_from_operand(dest));
			else
				Encoder().setge(GetLoweringContext().register_from_operand(dest));
			break;

		default:
			assert(false);
	}

	insn++;
	return true;
}


