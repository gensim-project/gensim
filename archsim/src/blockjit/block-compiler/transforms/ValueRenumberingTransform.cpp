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
	std::vector<IRRegId> renumberings;
	renumberings.resize(ctx.reg_count(), -1);

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		for(int i = 0; i < 6; ++i) {
			IROperand &op = insn->operands[i];
			if(op.is_vreg()) {
				if(renumberings[op.value] == -1)
					renumberings[op.value] = next_reg++;
				op.value = renumberings[op.value];
			}
		}
	}

	ctx.recount_regs(next_reg);

	return true;
}