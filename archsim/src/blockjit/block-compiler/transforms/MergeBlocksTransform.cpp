/*
 * MergeBlocksTransform.cpp
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

static void make_instruction_nop(IRInstruction *insn, bool set_block)
{
	insn->type = IRInstruction::NOP;
	insn->operands.clear();
	if(set_block) insn->ir_block = NOP_BLOCK;
}

MergeBlocksTransform::~MergeBlocksTransform()
{

}

bool MergeBlocksTransform::merge_block(TranslationContext &ctx, IRBlockId merge_from, IRBlockId merge_into)
{
	// Don't try to merge 'backwards' yet since it's a bit more complicated
	if(merge_from < merge_into) return false;

	unsigned int ir_idx = 0;
	// Nop out the terminator instruction from the merge_into block
	for(; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		// We can only merge if the terminator is a jmp
		if(insn->ir_block == merge_into && insn->type == IRInstruction::JMP) {
			make_instruction_nop(insn, true);
			break;
		}
	}

	for(; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;

		// Move instructions from the 'from' block to the 'to' block
		if(insn->ir_block == merge_from) insn->ir_block = merge_into;

		// TODO: this line causes a bug, but it would be a nice optimisation
		// to have.
		//if(insn->ir_block > merge_from) break;
	}
	return true;
}

bool MergeBlocksTransform::Apply(TranslationContext &ctx)
{
	tick_timer timer (0);
	timer.reset();
	std::vector<IRBlockId> succs (ctx.block_count(), -1);
	std::vector<int> pred_count (ctx.block_count(), 0);

	std::vector<IRBlockId> work_list;
	work_list.reserve(ctx.block_count());

	timer.tick("Init");

	SortIRTransform sort;
	sort.Apply(ctx);
	timer.tick("Sort");


	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;

		switch(insn->type) {
			case IRInstruction::JMP:
				//If a block ends in a jump, we should consider it a candidate for merging into
				work_list.push_back(insn->ir_block);
				succs[insn->ir_block] = insn->operands[0].value;
				pred_count[insn->operands[0].value]++;

				break;
			case IRInstruction::BRANCH:
				pred_count[insn->operands[1].value]++;
				pred_count[insn->operands[2].value]++;
				break;
			default:
				break;
		}
	}

	timer.tick("Identify");

	while(work_list.size()) {
		IRBlockId block = work_list.back();
		work_list.pop_back();

		// This block is only in the work list if it has only one successor, so we don't need to check for that

		// Look up the single successor of this block
		IRBlockId block_successor = succs[block];

		// If the successor has multiple predecessors, we can't merge it
		if(pred_count[block_successor] != 1) continue;

		if(!merge_block(ctx, block_successor, block)) continue;

		succs[block] = succs[block_successor];

		// If, post merging, the block has one successor, then put it on the work list
		if(succs[block] != (IRBlockId)-1) work_list.push_back(block);
	}

	timer.tick("Merge");
	timer.dump("MB");
	return true;
}
