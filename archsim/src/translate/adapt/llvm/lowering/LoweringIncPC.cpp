/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::translate::adapt;

bool BlockJITINCPCLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &amount = insn->operands[0];

	auto &PC = GetContext().GetArchDescriptor().GetRegisterFileDescriptor().GetTaggedEntry("PC");

	auto pc_ptr = GetContext().GetRegisterPointer(PC, 0);
	llvm::Value* pc_value = GetBuilder().CreateLoad(pc_ptr);
	pc_value = GetBuilder().CreateAdd(pc_value, GetValueFor(amount));
	GetBuilder().CreateStore(pc_value, pc_ptr);

	insn++;

	return true;
}
