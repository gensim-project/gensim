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
	bool not_sorted = false;
	IRBlockId id = 0;
	for(unsigned int i = 0; i < ctx.count(); ++i) {
		IRInstruction *insn = ctx.at(i);
		if(insn->ir_block < id) {
			not_sorted = true;
			break;
		}
		id = insn->ir_block;
	}
	if(!not_sorted) return true;

	algo::MergeSort sorter(ctx);
	auto new_buffer = sorter.perform_sort();
	if(new_buffer != ctx.get_ir_buffer()) {
		auto old_buffer = ctx.get_ir_buffer();
		ctx.set_ir_buffer(new_buffer);
		free(old_buffer);
	}
	return true;
}
