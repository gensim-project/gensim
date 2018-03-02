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

bool LowerCompare::Lower(const captive::shared::IRInstruction *&insn)
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

	auto dest_reg = &BLKJIT_TEMPS_0(dest->size);
	if(dest->is_alloc_stack()) {
		GetCompiler().encode_operand_to_reg(dest, *dest_reg);
	} else if(dest->is_alloc_reg()) {
		dest_reg = &GetCompiler().register_from_operand(dest);
	}
	
	switch (insn->type) {
		case IRInstruction::CMPEQ: {
			
			Encoder().sete(*dest_reg);

			if (should_branch) {
				assert(dest->is_alloc_reg());
				IRBlockId next_block = (next_insn+1)->ir_block;

				// Skip the next instruction (which is the branch)
				insn++;

				const IROperand *tt = &next_insn->operands[1];
				const IROperand *ft = &next_insn->operands[2];

				{
					uint32_t reloc_offset;
					Encoder().je_reloc(reloc_offset);
					GetLoweringContext().RegisterBlockRelocation(reloc_offset, tt->value);
				}

				{
					if(ft->value != next_block) {
						uint32_t reloc_offset;
						Encoder().jmp_reloc(reloc_offset);
						GetLoweringContext().RegisterBlockRelocation(reloc_offset, ft->value);
					}
				}
			}
			break;
		}
		case IRInstruction::CMPNE: {
			Encoder().setne(*dest_reg);

			if (should_branch) {
				assert(dest->is_alloc_reg());
				IRBlockId next_block = (next_insn+1)->ir_block;

				// Skip the next instruction (which is the branch)
				insn++;

				const IROperand *tt = &next_insn->operands[1];
				const IROperand *ft = &next_insn->operands[2];

				{
					uint32_t reloc_offset;
					Encoder().jne_reloc(reloc_offset);
					GetLoweringContext().RegisterBlockRelocation(reloc_offset, tt->value);
				}

				{
					if(ft->value != next_block) {
						uint32_t reloc_offset;
						Encoder().jmp_reloc(reloc_offset);
						GetLoweringContext().RegisterBlockRelocation(reloc_offset, ft->value);
					}
				}
			}

			break;
		}
		case IRInstruction::CMPLT:
			if (invert)
				Encoder().setnb(*dest_reg);
			else
				Encoder().setb(*dest_reg);
			break;

		case IRInstruction::CMPLTE:
			if (invert)
				Encoder().setnbe(*dest_reg);
			else
				Encoder().setbe(*dest_reg);
			break;

		case IRInstruction::CMPGT:
			if (invert)
				Encoder().setna(*dest_reg);
			else
				Encoder().seta(*dest_reg);
			break;

		case IRInstruction::CMPGTE:
			if (invert)
				Encoder().setnae(*dest_reg);
			else
				Encoder().setae(*dest_reg);
			break;

		default:
			assert(false);
	}
	
	if(dest->is_alloc_stack()) {
		Encoder().mov(*dest_reg, GetCompiler().stack_from_operand(dest));
	}

	insn++;
	return true;
}


