/*
 * RegStoreEliminationTransform.cpp
 *
 *  Created on: 13 Oct 2015
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

static void make_instruction_nop(IRInstruction *insn, bool set_block)
{
	insn->type = IRInstruction::NOP;
	insn->operands[0].type = IROperand::NONE;
	insn->operands[1].type = IROperand::NONE;
	insn->operands[2].type = IROperand::NONE;
	insn->operands[3].type = IROperand::NONE;
	insn->operands[4].type = IROperand::NONE;
	insn->operands[5].type = IROperand::NONE;
	if(set_block) insn->ir_block = NOP_BLOCK;
}

RegStoreEliminationTransform::~RegStoreEliminationTransform()
{

}

bool RegStoreEliminationTransform::Apply(TranslationContext &ctx)
{
	std::map<uint32_t, IRInstruction*> prev_writes;

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		auto &descr = insn_descriptors[insn->type];

		switch(insn->type) {
			case IRInstruction::JMP:
			case IRInstruction::BRANCH:
			case IRInstruction::READ_MEM:
			case IRInstruction::READ_MEM_USER:
			case IRInstruction::WRITE_MEM:
			case IRInstruction::WRITE_MEM_USER:
			case IRInstruction::LDPC:
			case IRInstruction::INCPC:
			case IRInstruction::TAKE_EXCEPTION:
			case IRInstruction::CALL:
			case IRInstruction::DISPATCH:
			case IRInstruction::RET:
			case IRInstruction::READ_DEVICE:
			case IRInstruction::WRITE_DEVICE:
			case IRInstruction::ADC_WITH_FLAGS:
				prev_writes.clear();
				break;

			case IRInstruction::READ_REG: {
				IROperand &offset = insn->operands[0];
				if(!offset.is_constant()) {
					prev_writes.clear();
					break;
				}

				assert(offset.is_constant());

				prev_writes.erase(offset.value);
				break;
			}
			case IRInstruction::WRITE_REG: {
				IROperand &offset = insn->operands[1];
				if(!offset.is_constant()) {
					prev_writes.clear();
					break;
				}

				assert(offset.is_constant());

				// Only nop out an instruction if the prev write is smaller than
				// the new one
				if(prev_writes.count(offset.value) && prev_writes.at(offset.value)->operands[0].size <= insn->operands[0].size) {
					make_instruction_nop(prev_writes.at(offset.value), false);
				}

				prev_writes[offset.value] = insn;

				break;
			}
			default:
				break;
		}
	}
	return true;
}

