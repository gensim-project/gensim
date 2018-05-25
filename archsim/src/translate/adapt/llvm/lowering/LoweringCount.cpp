/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITCOUNTLowering::Lower(const captive::shared::IRInstruction*& insn) {
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
