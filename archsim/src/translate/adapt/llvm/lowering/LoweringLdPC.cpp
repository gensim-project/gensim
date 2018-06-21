/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::translate::adapt;

bool BlockJITLDPCLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &target = insn->operands[0];

	assert(target.is_vreg());

	auto &PC = GetContext().GetArchDescriptor().GetRegisterFileDescriptor().GetTaggedEntry("PC");

	GetContext().SetValueFor(target, GetBuilder().CreateLoad(GetContext().GetRegisterPointer(PC, 0)));

	insn++;

	return true;
}
