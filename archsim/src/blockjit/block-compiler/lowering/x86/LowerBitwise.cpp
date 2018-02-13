/*
 * LowerBitwise.cpp
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

bool LowerBitwise::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->is_vreg()) {
		if (source->is_alloc_reg() && dest->is_alloc_reg()) {
			// OPER reg -> reg
			switch (insn->type) {
				case IRInstruction::AND:
					if (source->alloc_data != dest->alloc_data) {
						Encoder().andd(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
					}

					break;
				case IRInstruction::OR:
					if (source->alloc_data != dest->alloc_data) {
						Encoder().orr(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
					}

					break;
				case IRInstruction::XOR:
					Encoder().xorr(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
			// OPER stack -> reg
			switch (insn->type) {
				case IRInstruction::AND:
					Encoder().andd(GetCompiler().stack_from_operand(source), GetCompiler().register_from_operand(dest));
					break;
				case IRInstruction::OR:
					Encoder().orr(GetCompiler().stack_from_operand(source), GetCompiler().register_from_operand(dest));
					break;
				case IRInstruction::XOR:
					Encoder().xorr(GetCompiler().stack_from_operand(source), GetCompiler().register_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
			// OPER reg -> stack
			switch (insn->type) {
				case IRInstruction::AND:
					Encoder().andd(GetCompiler().register_from_operand(source), GetCompiler().stack_from_operand(dest));
					break;
				case IRInstruction::OR:
					Encoder().orr(GetCompiler().register_from_operand(source), GetCompiler().stack_from_operand(dest));
					break;
				case IRInstruction::XOR:
					Encoder().xorr(GetCompiler().register_from_operand(source), GetCompiler().stack_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
			// OPER stack -> stack
			const X86Register &temp = GetCompiler().get_temp(0, source->size);
			Encoder().mov(GetCompiler().stack_from_operand(source), temp);
			switch (insn->type) {
				case IRInstruction::AND:
					Encoder().andd(temp, GetCompiler().stack_from_operand(dest));
					break;
				case IRInstruction::OR:
					Encoder().orr(temp, GetCompiler().stack_from_operand(dest));
					break;
				case IRInstruction::XOR:
					Encoder().xorr(temp, GetCompiler().stack_from_operand(dest));
					break;
				default:
					assert(false);
					break;
			}
		} else {
			assert(false);
		}
	} else if (source->is_constant()) {
		if (dest->is_alloc_reg()) {
			// OPER const -> reg

			if (source->size == 8) {
				auto& tmp = GetCompiler().get_temp(0, 8);
				Encoder().mov(source->value, tmp);

				switch (insn->type) {
					case IRInstruction::AND:
						Encoder().andd(tmp, GetCompiler().register_from_operand(dest));
						break;
					case IRInstruction::OR:
						Encoder().orr(tmp, GetCompiler().register_from_operand(dest));
						break;
					case IRInstruction::XOR:
						Encoder().xorr(tmp, GetCompiler().register_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			} else {
				switch (insn->type) {
					case IRInstruction::AND:
						Encoder().andd(source->value, GetCompiler().register_from_operand(dest));
						break;
					case IRInstruction::OR:
						Encoder().orr(source->value, GetCompiler().register_from_operand(dest));
						break;
					case IRInstruction::XOR:
						Encoder().xorr(source->value, GetCompiler().register_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			}
		} else if (dest->is_alloc_stack()) {
			// OPER const -> stack

			if(source->size < 8) {
				switch (insn->type) {
					case IRInstruction::AND:
						Encoder().andd(source->value, dest->size, GetCompiler().stack_from_operand(dest));
						break;
					case IRInstruction::OR:
						Encoder().orr(source->value, dest->size, GetCompiler().stack_from_operand(dest));
						break;
					case IRInstruction::XOR:
						Encoder().xorr(source->value, dest->size, GetCompiler().stack_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			} else {
				Encoder().mov(source->value, BLKJIT_TEMPS_0(8));
				switch(insn->type) {
					case IRInstruction::AND:
						Encoder().andd(BLKJIT_TEMPS_0(8), GetCompiler().stack_from_operand(dest));
						break;
					case IRInstruction::OR:
						Encoder().orr(BLKJIT_TEMPS_0(8), GetCompiler().stack_from_operand(dest));
						break;
					case IRInstruction::XOR:
						Encoder().xorr(BLKJIT_TEMPS_0(8), GetCompiler().stack_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
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


