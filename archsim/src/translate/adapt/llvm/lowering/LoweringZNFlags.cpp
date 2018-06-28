/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITZNFLAGSLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &value = insn->operands[0];

	auto z_ptr = GetContext().GetTaggedRegisterPointer("Z");
	auto n_ptr = GetContext().GetTaggedRegisterPointer("N");

	auto in_value = GetValueFor(value);
	auto z_value = GetBuilder().CreateICmpEQ(in_value, llvm::ConstantInt::get(in_value->getType(), 0));
	auto n_value = GetBuilder().CreateICmpSLT(in_value, llvm::ConstantInt::get(in_value->getType(), 0));

	z_value = GetBuilder().CreateZExtOrTrunc(z_value, z_ptr->getType()->getPointerElementType());
	n_value = GetBuilder().CreateZExtOrTrunc(n_value, n_ptr->getType()->getPointerElementType());

	GetBuilder().CreateStore(z_value, z_ptr);
	GetBuilder().CreateStore(n_value, n_ptr);

	insn++;

	return true;
}
