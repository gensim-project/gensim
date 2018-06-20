/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITCOUNTLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &counter = insn->operands[0];
	const auto &amount = insn->operands[1];

	llvm::Value *counter_ptr = GetContext().GetValueFor(counter);
	counter_ptr = GetBuilder().CreateIntToPtr(counter_ptr, llvm::Type::getInt64PtrTy(GetContext().GetLLVMContext()));
	llvm::Value *counter_value = GetBuilder().CreateLoad(counter_ptr);
	counter_value = GetBuilder().CreateAdd(counter_value, GetValueFor(amount));
	GetBuilder().CreateStore(counter_value, counter_ptr);

	insn++;

	return true;
}
