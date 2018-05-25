/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITBRANCHLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &cond = insn->operands[0];
	const auto &tt = insn->operands[1];
	const auto &ft = insn->operands[2];
	
	auto cond_pred = GetBuilder().CreateZExtOrTrunc(GetValueFor(cond), llvm::Type::getInt1Ty(GetContext().GetLLVMContext()));
	GetBuilder().CreateCondBr(cond_pred, GetContext().GetLLVMBlock(tt.value), GetContext().GetLLVMBlock(ft.value));
	
	insn++;
	
	return true;
}
