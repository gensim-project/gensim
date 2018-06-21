/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITJMPLowering::Lower(const captive::shared::IRInstruction*& insn)
{

	const auto &target = insn->operands[0];
	assert(target.is_block());

	GetBuilder().CreateBr(GetContext().GetLLVMBlock(target.value));

	insn++;

	return true;
}
