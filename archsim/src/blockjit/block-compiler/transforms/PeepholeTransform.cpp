/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PeepholeTransform.cpp
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


// If this is an add of a negative value, replace it with a subtract of a positive value
static bool peephole_add(IRInstruction &insn)
{
	IROperand &op1 = insn.operands[0];
	if(op1.is_constant()) {
		uint64_t unsigned_negative = 0;
		switch(op1.size) {
			case 1:
				unsigned_negative = 0x80;
				break;
			case 2:
				unsigned_negative = 0x8000;
				break;
			case 4:
				unsigned_negative = 0x80000000;
				break;
			case 8:
				unsigned_negative = 0x8000000000000000;
				break;
			default:
				assert(false);
		}

		if(op1.value >= unsigned_negative) {
			insn.type = IRInstruction::SUB;
			switch(op1.size) {
				case 1:
					op1.value = -(int8_t)op1.value;
					break;
				case 2:
					op1.value = -(int16_t)op1.value;
					break;
				case 4:
					op1.value = -(int32_t)op1.value;
					break;
				case 8:
					op1.value = -(int64_t)op1.value;
					break;
				default:
					assert(false);
			}
		}
	}
	return true;
}

// If we shift an n-bit value by n bits, then replace this instruction with a move of 0.
// We explicitly move in 0, rather than XORing out, in order to avoid creating spurious use-defs
static bool peephole_shift(IRInstruction &insn)
{
	IROperand &op1 = insn.operands[0];
	IROperand &op2 = insn.operands[1];
	if(op1.is_constant()) {
		bool zeros_out = false;
		switch(op2.size) {
			case 1:
				zeros_out = op1.value == 8;
				break;
			case 2:
				zeros_out = op1.value == 16;
				break;
			case 4:
				zeros_out = op1.value == 32;
				break;
			case 8:
				zeros_out = op1.value == 64;
				break;
		}

		if(zeros_out) {
			insn.type = IRInstruction::MOV;
			op1.type = IROperand::CONSTANT;
			op1.value = 0;
		}
	}

	return true;
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
			insn->make_nop();
		}
	}
}

PeepholeTransform::~PeepholeTransform()
{

}

bool PeepholeTransform::Apply(TranslationContext &ctx)
{
	IRInstruction *prev_pc_inc = NULL;
	IRBlockId prev_block = INVALID_BLOCK_ID;

	// ARM HAX
	uint32_t pc_offset = 0x3c;

	for (unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		if(insn->ir_block == NOP_BLOCK) continue;

		if(insn->ir_block != prev_block) {
			prev_pc_inc = NULL;
			prev_block = insn->ir_block;
		}

		switch (insn->type) {
			case IRInstruction::ADD:
				if (insn->operands[0].is_constant() && insn->operands[0].value == 0) {
					insn->make_nop();
				} else {
					peephole_add(*insn);
				}

				break;

			case IRInstruction::SHR:
			case IRInstruction::SHL:
				if (insn->operands[0].is_constant() && insn->operands[0].value == 0) {
					insn->make_nop();
				} else {
					peephole_shift(*insn);
				}
				break;

			case IRInstruction::SUB:
			case IRInstruction::SAR:
			case IRInstruction::OR:
			case IRInstruction::XOR:
				if (insn->operands[0].is_constant() && insn->operands[0].value == 0) {
					insn->make_nop();
				}
				break;


			case IRInstruction::INCPC:
				if(prev_pc_inc) {
					insn->operands[0].value += prev_pc_inc->operands[0].value;
					prev_pc_inc->make_nop();
				}
				prev_pc_inc = insn;

				break;

			case IRInstruction::READ_REG:
				// HACK HACK HACK (pc offset)
				if(insn->operands[0].value == pc_offset) {
					prev_pc_inc = NULL;
				}
				break;

			case IRInstruction::RET:
			case IRInstruction::VERIFY:
			case IRInstruction::DISPATCH:
			case IRInstruction::READ_MEM:
			case IRInstruction::WRITE_MEM:
			case IRInstruction::LDPC:
			case IRInstruction::READ_DEVICE:
			case IRInstruction::WRITE_DEVICE:
			case IRInstruction::TAKE_EXCEPTION:
				prev_pc_inc = NULL;
				break;

			default:
				break;
		}

	}

	return true;
}
