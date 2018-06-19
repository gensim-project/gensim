/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

llvm::IRBuilder<>& BlockJITAdaptorLowerer::GetBuilder() {
	return GetContext().GetBuilder();
}

BlockJITLoweringContext& BlockJITAdaptorLowerer::GetContext() {
	return (BlockJITLoweringContext&) GetLoweringContext();
}

llvm::Value* BlockJITAdaptorLowerer::GetValueFor(const captive::shared::IROperand& operand) {
	return GetContext().GetValueFor(operand);
}

void BlockJITAdaptorLowerer::SetValueFor(const captive::shared::IROperand& operand, llvm::Value* value) {
	GetContext().SetValueFor(operand, value);
}

llvm::Value* BlockJITAdaptorLowerer::GetThreadPtr()
{
	return GetContext().GetThreadPtr();
}
