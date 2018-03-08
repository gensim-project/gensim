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
#include "blockjit/blockjit-abi.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerExtend::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (source->is_vreg()) {
		
		if (source->size == 4 && dest->size == 8) {
			if (insn->type == IRInstruction::ZX) {
				if (source->is_alloc_reg() && dest->is_alloc_reg()) {
					Encoder().mov(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest, 4));
				} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
					Encoder().mov(GetCompiler().stack_from_operand(source), GetCompiler().register_from_operand(dest, 4));
				} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
					Encoder().mov(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(source));
					Encoder().mov(GetCompiler().register_from_operand(source, 8), GetCompiler().stack_from_operand(dest));
				} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
					Encoder().mov(GetCompiler().stack_from_operand(source), BLKJIT_TEMPS_0(4));
					Encoder().mov(BLKJIT_TEMPS_0(8), GetCompiler().stack_from_operand(dest));
				} else {
					assert(false);
				}
			} else {
				auto source_reg = source->is_alloc_reg() ? GetCompiler().register_from_operand(source) : BLKJIT_TEMPS_0(source->size);
				auto dest_reg = dest->is_alloc_reg() ? GetCompiler().register_from_operand(dest) : BLKJIT_TEMPS_1(dest->size);
				
				if(source->is_alloc_stack()) {
					Encoder().mov(GetCompiler().stack_from_operand(source), source_reg);
				}
				
				Encoder().movsx(source_reg, dest_reg);
				
				if(dest->is_alloc_stack()) {
					Encoder().mov(dest_reg, GetCompiler().stack_from_operand(dest));
				}
			}
		} else {
			if (source->is_alloc_reg() && dest->is_alloc_reg()) {
				switch (insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
						break;

					case IRInstruction::SX:
						if (GetCompiler().register_from_operand(source) == REG_EAX && GetCompiler().register_from_operand(dest) == REG_RAX) {
							Encoder().cltq();
						} else if (GetCompiler().register_from_operand(source) == REG_AX && GetCompiler().register_from_operand(dest) == REG_EAX) {
							Encoder().cwtl();
						} else if (GetCompiler().register_from_operand(source) == REG_AL && GetCompiler().register_from_operand(dest) == REG_AX) {
							Encoder().cbtw();
						} else {
							Encoder().movsx(GetCompiler().register_from_operand(source), GetCompiler().register_from_operand(dest));
						}

						break;
					default:
						assert(false);
						break;
				}
			} else if (source->is_alloc_reg() && dest->is_alloc_stack()) {
				auto tmpreg = GetCompiler().get_temp(0, dest->size);

				switch(insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(GetCompiler().register_from_operand(source), tmpreg);
						break;
					case IRInstruction::SX:
						Encoder().movsx(GetCompiler().register_from_operand(source), tmpreg);
						break;
					default:
						assert(false);
						break;
				}
				Encoder().mov(tmpreg, GetCompiler().stack_from_operand(dest));

			} else if (source->is_alloc_stack() && dest->is_alloc_reg()) {
				auto& tmpreg = GetCompiler().unspill_temp(source, 0);

				switch (insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(tmpreg, GetCompiler().register_from_operand(dest));
						break;

					case IRInstruction::SX:
						Encoder().movsx(tmpreg, GetCompiler().register_from_operand(dest));
						break;
					default:
						assert(false);
						break;
				}
			} else if (source->is_alloc_stack() && dest->is_alloc_stack()) {
				auto& tmpreg = GetCompiler().unspill_temp(source, 0);

				switch (insn->type) {
					case IRInstruction::ZX:
						Encoder().movzx(tmpreg, GetCompiler().get_temp(0, dest->size));
						Encoder().mov(GetCompiler().get_temp(0, dest->size), GetCompiler().stack_from_operand(dest));
						break;

					case IRInstruction::SX:
						Encoder().movsx(tmpreg, GetCompiler().get_temp(0, dest->size));
						Encoder().mov(GetCompiler().get_temp(0, dest->size), GetCompiler().stack_from_operand(dest));
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
//		LC_WARNING(LogBlockJit) << "Constant extended into register in block " << std::hex << GetCompiler().GetBlockPA();
		Encoder().mov(source->value, GetCompiler().register_from_operand(dest));
	} else {
		assert(false);
	}

	insn++;
	return true;
}


