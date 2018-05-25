/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITSCMLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &new_mode  = insn->operands[0];
	
	auto mode_ptr = GetContext().GetStateBlockEntryPtr("ModeID", llvm::Type::getInt32Ty(GetContext().GetLLVMContext()));
	auto value = GetValueFor(new_mode);
	
	value = GetBuilder().CreateZExtOrTrunc(value, llvm::Type::getInt32Ty(GetContext().GetLLVMContext()));
	
	GetBuilder().CreateStore(value, mode_ptr);
	
	insn++;
	
	return true;
}
