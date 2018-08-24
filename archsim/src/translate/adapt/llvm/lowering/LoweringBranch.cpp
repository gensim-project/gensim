/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITBRANCHLowering::Lower(const captive::shared::IRInstruction*& insn)
{

	const auto &cond = insn->operands[0];
	const auto &tt = insn->operands[1];
	const auto &ft = insn->operands[2];

	auto cond_pred = GetBuilder().CreateZExtOrTrunc(GetValueFor(cond), llvm::Type::getInt1Ty(GetContext().GetLLVMContext()));
	GetBuilder().CreateCondBr(cond_pred, GetContext().GetLLVMBlock(tt.value), GetContext().GetLLVMBlock(ft.value));

	insn++;

	return true;
}
