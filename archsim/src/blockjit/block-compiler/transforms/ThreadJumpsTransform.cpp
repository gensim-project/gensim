/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ThreadJumpsTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

ThreadJumpsTransform::~ThreadJumpsTransform()
{

}

bool ThreadJumpsTransform::Apply(TranslationContext &ctx)
{
	tick_timer timer(0);
	timer.reset();

	std::vector<IRInstruction*> first_instructions(ctx.block_count(), NULL);
	std::vector<IRInstruction*> last_instructions(ctx.block_count(), NULL);

	timer.tick("Init");

	// Build up a list of the first instructions in each block.
	IRBlockId current_block_id = INVALID_BLOCK_ID;
	for (unsigned int ir_idx = 0; ir_idx < ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);

		if (insn->ir_block != current_block_id) {
			if (ir_idx > 0) {
				last_instructions[current_block_id] = ctx.at(ir_idx - 1);
			}

			current_block_id = insn->ir_block;
			first_instructions[current_block_id] = insn;
		}
	}

	timer.tick("Analysis");

	for(uint32_t block_id = 0; block_id < last_instructions.size(); ++block_id) {
		IRInstruction *source_instruction = last_instructions[block_id];
		if(!source_instruction) continue;

		switch(source_instruction->type) {
			case IRInstruction::JMP: {
				IRInstruction *target_instruction = first_instructions[source_instruction->operands[0].value];

				while (target_instruction->type == IRInstruction::JMP) {
					target_instruction = first_instructions[target_instruction->operands[0].value];
				}

				if (target_instruction->type == IRInstruction::RET) {
					//*source_instruction = *target_instruction;
					//source_instruction->ir_block = block_id;
				} else if (target_instruction->type == IRInstruction::BRANCH) {
					*source_instruction = *target_instruction;
					source_instruction->ir_block = block_id;
					goto do_branch_thread;
					//} else if (target_instruction->type == IRInstruction::DISPATCH) {
					//	*source_instruction = *target_instruction;
					//	source_instruction->ir_block = block_id;
				} else {
					source_instruction->operands[0].value = target_instruction->ir_block;
				}

				break;
			}

			case IRInstruction::BRANCH: {
do_branch_thread:
				IRInstruction *true_target = first_instructions[source_instruction->operands[1].value];
				IRInstruction *false_target = first_instructions[source_instruction->operands[2].value];

				while (true_target->type == IRInstruction::JMP) {
					true_target = first_instructions[true_target->operands[0].value];
				}

				while (false_target->type == IRInstruction::JMP) {
					false_target = first_instructions[false_target->operands[0].value];
				}

				source_instruction->operands[1].value = true_target->ir_block;
				source_instruction->operands[2].value = false_target->ir_block;

				break;
			}

			case IRInstruction::DISPATCH:
			case IRInstruction::RET:
				break;
			default:
				assert(false);
		}
	}

	timer.tick("Threading");
	timer.dump("Jump Threading");

	return true;
}
