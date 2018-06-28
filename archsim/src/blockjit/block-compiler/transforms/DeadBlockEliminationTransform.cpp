/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * DeadBlockEliminationTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <vector>
#include <set>
#include <cstdint>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

DeadBlockEliminationTransform::~DeadBlockEliminationTransform()
{

}

bool DeadBlockEliminationTransform::Apply(TranslationContext &ctx)
{
	tick_timer timer(0);
	timer.reset();

	std::vector<bool> live_blocks (ctx.block_count(), false);
	live_blocks[0] = true;

	timer.tick("Init");

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);

		switch(insn->type) {
			case IRInstruction::JMP:
				live_blocks[insn->operands[0].value] = true;
				break;
			case IRInstruction::BRANCH:
				live_blocks[insn->operands[1].value] = true;
				live_blocks[insn->operands[2].value] = true;
				break;
			default:
				break;
		}
	}

	timer.tick("Liveness");

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(!live_blocks[insn->ir_block]) {
			insn->make_nop();
		}
	}

	timer.tick("Killing");
	timer.dump("DBE");

	return true;
}


