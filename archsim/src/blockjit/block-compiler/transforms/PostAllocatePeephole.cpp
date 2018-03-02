/*
 * PostAllocateDCE.cpp
 *
 *  Created on: 30/6/2016
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <map>
#include <vector>
#include <set>
#include <cstdint>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

PostAllocatePeephole::~PostAllocatePeephole()
{

}

static void peephole_and(IRInstruction *insn)
{
	IROperand &constant = insn->operands[0];

	if(constant.is_constant()) {
		uint32_t op_size = 0;
		switch(constant.value) {
			case 0xff:
				op_size = 1;
				break;
			case 0xffff:
				op_size = 2;
				break;
			case 0xffffffff:
				op_size = 4;
				break;
			default:
				return;
		}

		if(constant.size != op_size) {
			insn->type = IRInstruction::ZX;
			insn->operands[0] = insn->operands[1];
			insn->operands[0].size = op_size;
		} else {
			insn->type = IRInstruction::NOP;
			insn->ir_block = NOP_BLOCK;
		}
	}
}

bool PostAllocatePeephole::Apply(TranslationContext& ctx)
{
	for (unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		if(insn->ir_block == NOP_BLOCK) continue;

		if(insn->type == IRInstruction::AND) peephole_and(insn);

	}

	return true;
}