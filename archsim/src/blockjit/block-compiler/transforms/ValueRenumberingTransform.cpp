/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <cstdint>
#include <vector>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

ValueRenumberingTransform::~ValueRenumberingTransform()
{

}

bool ValueRenumberingTransform::Apply(TranslationContext& ctx)
{
	IRRegId next_reg = 0;
	IRRegId invalid_renumbering = 0xffffffff;
	std::vector<IRRegId> renumberings;
	renumberings.resize(ctx.reg_count(), invalid_renumbering);

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		for(unsigned i = 0; i < insn->operands.size(); ++i) {
			IROperand &op = insn->operands[i];
			if(op.is_vreg()) {
				if(renumberings[op.value] == invalid_renumbering)
					renumberings[op.value] = next_reg++;
				op.value = renumberings[op.value];
			}
		}
	}

	ctx.recount_regs(next_reg);

	return true;
}
