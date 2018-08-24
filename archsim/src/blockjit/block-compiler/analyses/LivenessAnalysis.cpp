/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LivenessAnalysis.cpp
 *
 *  Created on: 22 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/analyses/Analysis.h"

using namespace captive::arch::jit::analyses;
using namespace captive::arch::jit;
using namespace captive::shared;

HostRegLivenessData HostRegLivenessAnalysis::Run(TranslationContext &ctx)
{
	HostRegLivenessData data;

	uint32_t live_regs = 0;
	IRBlockId curr_block = NOP_BLOCK;

	for(int32_t insn_idx = ctx.count()-1; insn_idx >= 0; insn_idx--) {
		const IRInstruction &insn = *ctx.at(insn_idx);
		const insn_descriptor &descr = insn_descriptors[insn.type];

		if(insn.ir_block == NOP_BLOCK) {
			continue;
		}

		if(insn.ir_block != curr_block) {
			live_regs = 0;
			curr_block = insn.ir_block;
		}

		data.live_out[&insn] = live_regs;

		for(uint32_t op_idx = 0; op_idx < 7; ++op_idx) {
			const IROperand &op = insn.operands[op_idx];
			char op_type = descr.format[op_idx];

			if(op.is_alloc_reg()) {
				if(op_type == 'O') {
					live_regs &= ~(1 << op.alloc_data);
				}
			}
		}

		for(uint32_t op_idx = 0; op_idx < 7; ++op_idx) {
			const IROperand &op = insn.operands[op_idx];
			char op_type = descr.format[op_idx];

			if(op.is_alloc_reg()) {
				if(op_type == 'I' || op_type == 'B') {
					live_regs |= 1 << op.alloc_data;
				}
			}
		}
	}

	return data;
}
