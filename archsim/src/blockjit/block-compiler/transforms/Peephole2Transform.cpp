/*
 * Peephole2Transform.cpp
 *
 *  Created on: 30 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <vector>
#include <set>
#include <cstdint>
#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

Peephole2Transform::~Peephole2Transform()
{

}

bool Peephole2Transform::Apply(TranslationContext &ctx)
{
	IRInstruction *prev_pc_inc = NULL;
	IRBlockId prev_block = INVALID_BLOCK_ID;

	for (unsigned int ir_idx = 0; ir_idx < ctx.count()-3; ++ir_idx) {

		// Check for an instruction combination of the form
		// ldreg XXX, (r0)
		// mov (r0), (r1)
		// ldreg YYY, (r0)
		//
		// and convert it to the form
		//
		// ldreg XXX (r1)
		// ldreg YYY (r0)

		IRInstruction *ldreg1 = ctx.at(ir_idx);
		IRInstruction *mov    = ctx.at(ir_idx+1);
		IRInstruction *ldreg2 = ctx.at(ir_idx+2);

		if(ldreg1->type == IRInstruction::READ_REG && mov->type == IRInstruction::MOV && ldreg2->type == IRInstruction::READ_REG) {
			auto &ldreg1_dest = ldreg1->operands[1];
			auto &mov_src = mov->operands[0];
			auto &mov_dest = mov->operands[1];
			auto &ldreg2_dest = ldreg2->operands[1];

			if(ldreg1_dest.is_alloc_reg() && mov_src.is_alloc_reg() && mov_dest.is_alloc_reg() && ldreg2_dest.is_alloc_reg()) {
				if(ldreg1_dest.alloc_data == mov_src.alloc_data && ldreg1_dest.alloc_data == ldreg2_dest.alloc_data) {
//					fprintf(stderr, "LOL\n");
					ldreg1_dest.alloc_data = mov_dest.alloc_data;

					mov->make_nop();
				}

			}
		}

	}

	return true;
}
