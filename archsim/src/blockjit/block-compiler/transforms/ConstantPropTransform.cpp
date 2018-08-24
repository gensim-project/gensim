/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ConstantPropTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include <wutils/tick-timer.h>

#include <vector>
#include <set>
#include <cstdint>
#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

ConstantPropTransform::~ConstantPropTransform()
{

}

bool ConstantPropTransform::Apply(TranslationContext &ctx)
{
	std::map<IRRegId, IROperand> constant_operands;

	IRBlockId prev_block = 0;

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;
		if(prev_block != insn->ir_block) {
			constant_operands.clear();
		}
		prev_block = insn->ir_block;

		if(insn->type == IRInstruction::MOV) {
			assert(insn->operands[1].is_vreg());
			if(insn->operands[0].type == IROperand::CONSTANT) {
				constant_operands[insn->operands[1].value] = insn->operands[0];
				continue;
			}
		}

		if(insn->type == IRInstruction::ADD || insn->type == IRInstruction::SUB) {
			assert(insn->operands[1].is_vreg());
			if(insn->operands[0].type == IROperand::CONSTANT) {
				if(constant_operands.count(insn->operands[1].value)) {
					if(insn->type == IRInstruction::ADD) {
						constant_operands[insn->operands[1].value].value += insn->operands[0].value;
					} else {
						constant_operands[insn->operands[1].value].value -= insn->operands[0].value;
					}
				}
			}
		}

		struct insn_descriptor &descr = insn_descriptors[insn->type];

		for(unsigned int op_idx = 0; op_idx < insn->operands.size(); ++op_idx) {
			IROperand *operand = &(insn->operands[op_idx]);
			if(descr.format[op_idx] == 'I') {
				if(operand->is_vreg() && constant_operands.count(operand->value)) {
					*operand = constant_operands[operand->value];
				}
			} else {
				if(operand->is_vreg()) {
					constant_operands.erase(operand->value);
				}
			}
		}
	}

	return true;
}
