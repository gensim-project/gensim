/*
 * LowerMov.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerMov::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->type == IROperand::VREG) {
		// mov vreg -> vreg
		if (source->is_alloc_reg()) {
			auto& src_reg = GetCompiler().register_from_operand(source);

			if (dest->is_alloc_reg()) {
				// mov reg -> reg

				if (source->alloc_data != dest->alloc_data) {
					Encoder().mov(src_reg, GetCompiler().register_from_operand(dest));
				}
			} else if (dest->is_alloc_stack()) {
				// mov reg -> stack
				Encoder().mov(src_reg, GetCompiler().stack_from_operand(dest));
			} else {
				assert(false);
			}
		} else if (source->is_alloc_stack()) {
			const arch::x86::X86Memory src_mem = GetCompiler().stack_from_operand(source);

			if (dest->is_alloc_reg()) {
				// mov stack -> reg
				Encoder().mov(src_mem, GetCompiler().register_from_operand(dest));
			} else if (dest->is_alloc_stack()) {
				// mov stack -> stack

				if (source->alloc_data != dest->alloc_data) {
					auto& tmp = GetCompiler().get_temp(0, source->size);
					Encoder().mov(src_mem, tmp);
					Encoder().mov(tmp, GetCompiler().stack_from_operand(dest));
				}
			} else {
				assert(false);
			}
		} else {
			assert(false);
		}
	} else if (source->type == IROperand::CONSTANT) {
		// mov const -> vreg

		if (dest->is_alloc_reg()) {
			// mov imm -> reg

			if (source->value == 0) {
				Encoder().xorr(GetCompiler().register_from_operand(dest), GetCompiler().register_from_operand(dest));
			} else {
				Encoder().mov(source->value, GetCompiler().register_from_operand(dest));
			}
		} else if (dest->is_alloc_stack()) {
			if(source->value == 0) {
				auto temp_reg = GetCompiler().get_temp(0, dest->size);
				Encoder().xorr(temp_reg, temp_reg);
				Encoder().mov(temp_reg, GetCompiler().stack_from_operand(dest));
			} else {
				// mov imm -> stack
				switch (dest->size) {
					case 1:
						Encoder().mov1(source->value, GetCompiler().stack_from_operand(dest));
						break;
					case 2:
						Encoder().mov2(source->value, GetCompiler().stack_from_operand(dest));
						break;
					case 4:
						Encoder().mov4(source->value, GetCompiler().stack_from_operand(dest));
						break;
					case 8:
						Encoder().mov(source->value, BLKJIT_TEMPS_0(8));
						Encoder().mov(BLKJIT_TEMPS_0(8), GetCompiler().stack_from_operand(dest));
						break;
					default:
						assert(false);
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

