/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ReorderBlocksTransform.cpp
 *
 *  Created on: 7 Oct 2015
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

ReorderBlocksTransform::~ReorderBlocksTransform()
{

}

bool ReorderBlocksTransform::Apply(TranslationContext &ctx)
{
	tick_timer timer(0);
	timer.reset();

	std::vector<IRBlockId> reordering (ctx.block_count(), NOP_BLOCK);
	std::vector<std::pair<IRBlockId, IRBlockId> > block_targets (ctx.block_count(), { NOP_BLOCK, NOP_BLOCK} );
	std::set<IRBlockId> blocks;

	timer.tick("Init");

	blocks.insert(0);
	// build a simple cfg
	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		switch(insn->type) {
			case IRInstruction::JMP:
				block_targets[insn->ir_block] = { insn->operands[0].value, NOP_BLOCK };
				blocks.insert(insn->operands[0].value);
				break;
			case IRInstruction::BRANCH:
				block_targets[insn->ir_block] = { insn->operands[1].value, insn->operands[2].value };
				blocks.insert(insn->operands[1].value);
				blocks.insert(insn->operands[2].value);
				break;
			case IRInstruction::RET:
			case IRInstruction::DISPATCH:
				block_targets[insn->ir_block] = { NOP_BLOCK, NOP_BLOCK };
				break;
			default:
				break;
		}
	}

	timer.tick("CFG");

	// For each block, figure out the maximum number of predecessors which it has
	std::vector<uint32_t> max_depth (ctx.block_count(), 0);
	std::vector<IRBlockId> work_list;

	work_list.reserve(10);
	work_list.push_back(0);
	while(work_list.size()) {
		IRBlockId block = work_list.back();
		work_list.pop_back();

		if(block == NOP_BLOCK) continue;

		uint32_t my_max_depth = max_depth[block];
		IRBlockId first_target = block_targets[block].first;
		IRBlockId second_target = block_targets[block].second;

		if(first_target != NOP_BLOCK) {
			uint32_t prev_max_depth = max_depth[block_targets[block].first];
			if(prev_max_depth < my_max_depth+1) {
				max_depth[block_targets[block].first] = my_max_depth+1;
				work_list.push_back(block_targets[block].first);
			}
		}

		if(second_target != NOP_BLOCK) {
			uint32_t prev_max_depth = max_depth[block_targets[block].second];
			if(prev_max_depth < my_max_depth+1) {
				max_depth[block_targets[block].second] = my_max_depth+1;
				work_list.push_back(block_targets[block].second);
			}
		}
	}

	timer.tick("Analyse");

	// Oh god what have I done
	auto comp = [&max_depth] (IRBlockId a, IRBlockId b) -> bool { return max_depth[a] < max_depth[b]; };

	std::vector<IRBlockId> queue;
	queue.insert(queue.begin(), blocks.begin(), blocks.end());
	std::sort(queue.begin(), queue.end(), comp);

	for(uint32_t i = 0; i < queue.size(); ++i) {
		reordering[queue[i]] = i;
	}

	timer.tick("Sort");

	// FINALLY actually assign the new block ordering
	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;
		insn->ir_block = reordering[insn->ir_block];

		if(insn->type == IRInstruction::JMP) {
			insn->operands[0].value = reordering[insn->operands[0].value];
		}
		if(insn->type == IRInstruction::BRANCH) {
			insn->operands[1].value = reordering[insn->operands[1].value];
			insn->operands[2].value = reordering[insn->operands[2].value];
		}

	}

	ctx.recount_blocks(queue.size());

	timer.tick("Assign");
	timer.dump("Reorder ");

	return true;
}
