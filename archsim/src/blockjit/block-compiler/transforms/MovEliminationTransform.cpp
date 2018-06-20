/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "blockjit/block-compiler/transforms/Transform.h"

#include <vector>

using namespace captive::arch::jit::transforms;

MovEliminationTransform::~MovEliminationTransform() {
}

bool MovEliminationTransform::Apply(TranslationContext& ctx)
{
	std::vector<uint32_t> write_count(ctx.reg_count()), read_count(ctx.reg_count());
	std::vector<shared::IRInstruction *> reg_mov(ctx.reg_count());
	
	for(unsigned insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto instruction = ctx.at(insn_idx);
		auto &descriptor = instruction->descriptor();
		
		for(unsigned int op_idx = 0; op_idx < instruction->operands.size(); ++op_idx) {
			auto &operand = instruction->operands.at(op_idx);
			if(operand.is_vreg()) {
				if(instruction->descriptor().format[op_idx] == 'I') {
					read_count[operand.get_vreg_idx()]++;
				}
				if(instruction->descriptor().format[op_idx] == 'O') {
					write_count[operand.get_vreg_idx()]++;
				}
				if(instruction->descriptor().format[op_idx] == 'B') {
					read_count[operand.get_vreg_idx()]++;
					write_count[operand.get_vreg_idx()]++;
				}
			}
		}
		
		if(instruction->type == shared::IRInstruction::MOV) {
			if(instruction->operands.at(0).is_vreg()) {
				reg_mov[instruction->operands.at(0).get_vreg_idx()] = instruction;
			}
		}
	}
	
//	printf("*** starting mov eliminatin\n");
	
	std::vector<shared::IRRegId> replacements(ctx.reg_count(), 0xffffffff);
	for(unsigned reg_idx = 0; reg_idx < ctx.reg_count(); ++reg_idx) {
		if(write_count[reg_idx] == 1 && read_count[reg_idx] == 1 && reg_mov[reg_idx] != nullptr) {
			// we've got a candidate!
			shared::IRRegId replacement = reg_mov[reg_idx]->operands[1].get_vreg_idx();
			
//			printf("%u is a replacement for %u\n", replacement, reg_idx);
			replacements[reg_idx] = replacement;
			
			reg_mov[reg_idx]->make_nop();
		}
	}
	
	for(unsigned insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto instruction = ctx.at(insn_idx);
		auto &descriptor = instruction->descriptor();
		
		for(unsigned op_idx = 0; op_idx < instruction->operands.size(); ++op_idx) {
			auto &operand = instruction->operands.at(op_idx);
			if(operand.is_vreg()) {
				shared::IRRegId replacement = replacements[operand.get_vreg_idx()];
				if(replacements[operand.get_vreg_idx()] != 0xffffffff) {
					while(replacements[replacement] != 0xffffffff) {
						replacement = replacements[replacement];
					}
//					printf("identified %u as a replacement for %u\n", replacement, operand.get_vreg_idx());
					operand.value = replacement;
				}
			}
		}
	}
	
	return true;
}

