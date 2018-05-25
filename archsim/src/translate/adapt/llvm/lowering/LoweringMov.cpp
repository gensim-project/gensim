/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITMOVLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &from = insn->operands[0];
	const auto &to = insn->operands[1];
	
	SetValueFor(to, GetValueFor(from));
	
	insn++;
	
	return true;
}

bool BlockJITCMOVLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &predicate = insn->operands[0];
	const auto &yes = insn->operands[1];
	const auto &no = insn->operands[2];
	
	const auto &dest = insn->operands[2];
	
	auto pred_value = GetBuilder().CreateTrunc(GetValueFor(predicate), llvm::Type::getInt1Ty(GetContext().GetLLVMContext()));
	auto result_value = GetBuilder().CreateSelect(pred_value,GetValueFor(yes), GetValueFor(no));
	
	SetValueFor(dest, result_value);
	
	insn++;
	
	return true;
}
