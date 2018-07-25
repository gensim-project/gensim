/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/transforms/Transform.h"

#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

StackToRegTransform::StackToRegTransform(wutils::vbitset<> used_phys_regs) : used_phys_regs_(used_phys_regs)
{

}

StackToRegTransform::~StackToRegTransform()
{

}

wutils::vbitset<> StackToRegTransform::GetUsedPhysRegs() const
{
	return used_phys_regs_;
}

bool StackToRegTransform::Apply(TranslationContext& ctx)
{
	std::map<uint32_t, uint32_t> lowered_entries;
	wutils::vbitset<> avail_phys_regs = used_phys_regs_;
	avail_phys_regs.invert();

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		// Don't need to do any clever allocation here since the stack entries should already be re-used

		for(unsigned int op_idx = 0; op_idx < insn->operands.size(); ++op_idx) {
			IROperand &op = insn->operands[op_idx];
			if(!op.is_valid()) break;

			if(op.is_alloc_stack()) {
				int32_t selected_reg;
				if(!lowered_entries.count(op.alloc_data)) {
					if(avail_phys_regs.all_clear()) continue;

					selected_reg = avail_phys_regs.get_lowest_set();
					avail_phys_regs.set(selected_reg, 0);
					assert(selected_reg != -1);
					lowered_entries[op.alloc_data] = selected_reg;
					used_phys_regs_.set(selected_reg, 1);

				} else {
					selected_reg = lowered_entries[op.alloc_data];
				}
				op.allocate(IROperand::ALLOCATED_REG, selected_reg);
			}
		}
	}

	return true;
}
