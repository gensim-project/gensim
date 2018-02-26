/*
 * LowerReadReg.cpp
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

// This is complicated - register reads are frequent, so we want to emit them as efficiently as possible.
// Therefore, we try to emit as few instructions as possible for frequent cases (such as read-modify-write
// operations on a single register).

bool LowerReadReg::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *offset = &insn->operands[0];
	const IROperand *target = &insn->operands[1];

	const IRInstruction *mod_insn, *store_insn, *potential_killer;
	mod_insn = insn+1;
	store_insn = insn+2;

	potential_killer = insn+3;
	bool notfound = true;
	while(notfound) {
		switch(potential_killer->type) {
			case IRInstruction::INCPC:
			case IRInstruction::BARRIER:
				potential_killer++;
				break;
			default:
				notfound = false;
				break;
		}
	}


	// If these three instructions are a read-modify-write of the same register, then emit a modification of
	// the register, and then the normal read (eliminate the store)
	// e.g.
	// ldreg $0x0, v0
	// add $0x10, v0
	// streg v0, $0x0
	//  ||
	//  \/
	// add $0x10, $0x0(REGSTATE_REG)
	// mov $0x0(REGSTATE_REG), [v0]
	//
	// We also determine if the final mov can be eliminted. It is only eliminated if the following instruction
	// kills the value read from the register, e.g.
	//
	// add $0x10, $0x0(REGSTATE_REG)
	// mov $0x0(REGSTATE_REG), [v0]
	// mov $0x8(REGSTATE_REG), [v0]
	//  ||
	//  \/
	// add $0x10, $0x0(REGSTATE_REG)
	// mov $0x8(REGSTATE_REG), [v0]
	unsigned reg_offset = insn->operands[0].value;
	if(mod_insn->ir_block == insn->ir_block && store_insn->ir_block == insn->ir_block && store_insn->type == IRInstruction::WRITE_REG && store_insn->operands[1].value == reg_offset) {

		const IROperand *my_target =     &insn->operands[1];
		const IROperand *modify_source = &mod_insn->operands[0];
		const IROperand *modify_target = &mod_insn->operands[1];
		const IROperand *store_source  = &store_insn->operands[0];

		if(my_target->is_alloc_reg() && !modify_source->is_alloc_stack() && !modify_target->is_alloc_stack() && store_source->is_alloc_reg()) {

			unsigned my_target_reg = my_target->alloc_data;
			unsigned modify_target_reg = modify_target->alloc_data;
			unsigned store_source_reg = store_source->alloc_data;

			// TODO: this is hideous
			bool fused = false;
			if(my_target_reg == modify_target_reg && modify_target_reg == store_source_reg) {
				switch(mod_insn->type) {
					case IRInstruction::ADD:
						if(modify_source->is_constant()) {
							Encoder().add(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							Encoder().add(GetCompiler().register_from_operand(modify_source), X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::SUB:
						if(modify_source->is_constant()) {
							Encoder().sub(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							Encoder().sub(GetCompiler().register_from_operand(modify_source), X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::OR:
						if(modify_source->is_constant()) {
							Encoder().orr(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							Encoder().orr(GetCompiler().register_from_operand(modify_source), X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::AND:
						if(modify_source->is_constant()) {
							Encoder().andd(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							Encoder().andd(GetCompiler().register_from_operand(modify_source), X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::XOR:
						if(modify_source->is_constant()) {
							Encoder().xorr(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							Encoder().xorr(GetCompiler().register_from_operand(modify_source), X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::SHL:
						if(modify_source->is_constant()) {
							Encoder().shl(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							break;
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::SHR:
						if(modify_source->is_constant()) {
							Encoder().shr(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							break;
						} else {
							assert(false);
						}
						fused = true;
						break;
					case IRInstruction::ROR:
						if(modify_source->is_constant()) {
							Encoder().ror(modify_source->value, modify_source->size, X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset));
						} else if(modify_source->is_vreg()) {
							break;
						} else {
							assert(false);
						}
						fused = true;
						break;
					default:
						break;
				}

				if(fused) {
					insn += 3;

					// If the IR node after the fused instruction kills the register we would load into, then
					// we don't need to load the register value
					auto &descr = insn_descriptors[potential_killer->type];
					//~ printf("Killer is %s %x\n", descr.mnemonic, pa);
					bool kills_value = false;
					for(int i = 0; i < 6; ++i) {
						const IROperand *kop = &potential_killer->operands[i];
						// If the operand is an input of the register, we need the move
						if(descr.format[i] == 'I' || descr.format[i] == 'B') {
							if(kop->is_alloc_reg() && kop->alloc_data == store_source->alloc_data) {
								kills_value = false;
								//~ printf("doesn't kill\n");
								break;
							}
						}

						if(descr.format[i] == 'O') {
							if(kop->is_alloc_reg() && kop->alloc_data == store_source->alloc_data) {
								kills_value = true;
								//~ printf("kills\n");
								break;
							}
						}
					}

					// If the killer DOES kill the instruction, then we DO NOT need the move
					if(!kills_value) {
						Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, reg_offset), GetCompiler().register_from_operand(store_source));
					}
					return true;
				}
			}
		}
	}


	if (offset->is_constant()) {
		// Load a constant offset guest register into the storage location
		if (target->is_alloc_reg()) {
			Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, offset->value), GetCompiler().register_from_operand(target));
		} else if (target->is_alloc_stack()) {
			Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, offset->value), GetCompiler().get_temp(0, target->size));
			Encoder().mov(GetCompiler().get_temp(0, target->size), GetCompiler().stack_from_operand(target));
		} else {
			assert(false);
		}
	} else if (offset->is_alloc_reg()) {
		// Load a constant offset guest register into the storage location
		if (target->is_alloc_reg()) {
			Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, GetCompiler().register_from_operand(offset), 1), GetCompiler().register_from_operand(target));
		} else if (target->is_alloc_stack()) {
			Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, GetCompiler().register_from_operand(offset), 1), GetCompiler().get_temp(0, target->size));
			Encoder().mov(GetCompiler().get_temp(0, target->size), GetCompiler().stack_from_operand(target));
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}

	insn++;
	return true;
}



