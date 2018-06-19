/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <vector>
#include <set>
#include <cstdint>
#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

AllocationWriterTransform::AllocationWriterTransform(const allocations_t& allocations) : allocations_(allocations)
{

}


AllocationWriterTransform::~AllocationWriterTransform() {
}

bool AllocationWriterTransform::Apply(TranslationContext& ctx)
{
	for(uint32_t insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto insn = ctx.at(insn_idx);
		for(uint32_t op_idx = 0; op_idx < insn->operands.size(); ++op_idx) {
			auto &operand = insn->operands.at(op_idx);
			if(operand.is_vreg()) {
				IRRegId vreg_id = operand.get_vreg_idx();
				if(allocations_.count(vreg_id)) {
					operand.allocate(allocations_.at(vreg_id).first, allocations_.at(vreg_id).second);
				}
			}
		}
	}
	
	return true;
}
