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
#include "blockjit/blockjit-abi.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

// This class lowers compare instructions, and fuses compare-and-branch pairs where possible.

bool LowerCompareSigned::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *lhs = &insn->operands[0];
	const IROperand *rhs = &insn->operands[1];
	const IROperand *dest = &insn->operands[2];

	bool invert = false;

	switch (lhs->type) {
		case IROperand::VREG: {
			switch (rhs->type) {
				case IROperand::VREG: {
					if (lhs->is_alloc_reg() && rhs->is_alloc_reg()) {
						Encoder().cmp(GetCompiler().register_from_operand(rhs), GetCompiler().register_from_operand(lhs));
					} else if (lhs->is_alloc_stack() && rhs->is_alloc_reg()) {
						Encoder().cmp(GetCompiler().register_from_operand(rhs), GetCompiler().stack_from_operand(lhs));
					} else if (lhs->is_alloc_reg() && rhs->is_alloc_stack()) {
						// Apparently we can't yet encode cmp (stack), (reg) so encode cmp (reg), (stack) instead
						// and invert the result
						invert = true;

						Encoder().cmp(GetCompiler().register_from_operand(lhs), GetCompiler().stack_from_operand(rhs));
					} else if (lhs->is_alloc_stack() && rhs->is_alloc_stack()) {
						// Apparently we can't yet encode cmp (stack), (reg) so encode cmp (reg), (stack) instead
						// and invert the result
						invert = true;

						auto& tmp = GetCompiler().unspill_temp(lhs, 0);
						Encoder().cmp(tmp, GetCompiler().stack_from_operand(rhs));
					} else {
						assert(false);
					}

					break;
				}

				case IROperand::CONSTANT: {
					if (lhs->is_alloc_reg()) {
						Encoder().cmp(rhs->value, GetCompiler().register_from_operand(lhs));
					} else if (lhs->is_alloc_stack()) {
						switch (rhs->size) {
							case 1:
								Encoder().cmp1(rhs->value, GetCompiler().stack_from_operand(lhs));
								break;
							case 2:
								Encoder().cmp2(rhs->value, GetCompiler().stack_from_operand(lhs));
								break;
							case 4:
								Encoder().cmp4(rhs->value, GetCompiler().stack_from_operand(lhs));
								break;
							case 8:
								Encoder().cmp8(rhs->value, GetCompiler().stack_from_operand(lhs));
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
			invert = true;

			switch (rhs->type) {
				case IROperand::VREG: {
					if (rhs->is_alloc_reg()) {
						Encoder().cmp(lhs->value, GetCompiler().register_from_operand(rhs));
					} else if (rhs->is_alloc_stack()) {
						switch (lhs->size) {
							case 1:
								Encoder().cmp1(lhs->value, GetCompiler().stack_from_operand(rhs));
								break;
							case 2:
								Encoder().cmp2(lhs->value, GetCompiler().stack_from_operand(rhs));
								break;
							case 4:
								Encoder().cmp4(lhs->value, GetCompiler().stack_from_operand(rhs));
								break;
							case 8:
								Encoder().cmp8(lhs->value, GetCompiler().stack_from_operand(rhs));
								break;
						}
					} else {
						assert(false);
					}

					break;
				}
				case IROperand::CONSTANT: {
					LC_WARNING(LogBlockJit) << "Constant compared against constant in block " << std::hex << GetCompiler().GetBlockPA();
					Encoder().mov(rhs->value, GetCompiler().get_temp(0, rhs->size));
					Encoder().cmp(lhs->value, GetCompiler().get_temp(0, rhs->size));
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
			if (invert)
				Encoder().setnl(GetCompiler().register_from_operand(dest));
			else
				Encoder().setl(GetCompiler().register_from_operand(dest));
			break;

		case IRInstruction::CMPSLTE:
			if (invert)
				Encoder().setnle(GetCompiler().register_from_operand(dest));
			else
				Encoder().setle(GetCompiler().register_from_operand(dest));
			break;

		case IRInstruction::CMPSGT:
			if (invert)
				Encoder().setng(GetCompiler().register_from_operand(dest));
			else
				Encoder().setg(GetCompiler().register_from_operand(dest));
			break;

		case IRInstruction::CMPSGTE:
			if (invert)
				Encoder().setnge(GetCompiler().register_from_operand(dest));
			else
				Encoder().setge(GetCompiler().register_from_operand(dest));
			break;

		default:
			assert(false);
	}

	insn++;
	return true;
}


