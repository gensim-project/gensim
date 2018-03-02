/*
 * SortIRTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"
#include "blockjit/ir-sorter.h"

#include "util/wutils/tick-timer.h"

#include <vector>
#include <set>
#include <cstdint>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

SortIRTransform::~SortIRTransform()
{

}

bool SortIRTransform::Apply(TranslationContext &ctx)
{
	IRBlockId id = 0;
	
	// determine if 
	// a) the IR is already totally sorted
	// b) the IR is sorted, except for NOPs
	// c) the IR is not sorted
	
	bool ir_sorted = true;
	bool nops_sorted = true;
	
	bool found_nops = false;
	IRBlockId prev_block = ctx.at(0)->ir_block;
	
	for(unsigned int i = 0; i < ctx.count(); ++i) {
		IRInstruction *insn = ctx.at(i);
		IRBlockId block = insn->ir_block;
		
		// is this instruction a nop?
		if(block == NOP_BLOCK) {
			found_nops = true;
		} else {
			if(found_nops) {
				nops_sorted = false;
			}
			
			if(block < prev_block) {
				ir_sorted = false;
			}
			prev_block = block;
		}
	}
	
	IRInstruction * new_buffer = ctx.get_ir_buffer();
	if(!ir_sorted) {
		algo::MergeSort sorter(ctx);
		new_buffer = sorter.perform_sort();
	} else if(!nops_sorted) {
		algo::NopFilter sorter(ctx);
		new_buffer = sorter.perform_sort();
	}
	
	if(new_buffer != ctx.get_ir_buffer()) {
		auto old_buffer = ctx.get_ir_buffer();
		ctx.set_ir_buffer(new_buffer);
		free(old_buffer);
	}
	return true;
}
