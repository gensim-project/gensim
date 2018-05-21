/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITCLZLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = insn->operands[0];
	auto &dest = insn->operands[1];
	
	auto lhs_value = GetValueFor(lhs);
	
	auto i1_type = llvm::Type::getIntNTy(GetContext().GetLLVMContext(), 1);
	auto param_type = llvm::Type::getIntNTy(GetContext().GetLLVMContext(), lhs.size*8);
	std::string function_name = "llvm.ctlz." + std::to_string(lhs.size * 8);
	
	auto is_zero_undef = llvm::ConstantInt::get(i1_type, 0, false);
	auto function = GetContext().GetModule()->getOrInsertFunction(function_name, param_type, param_type, i1_type);
	auto value = GetBuilder().CreateCall(function, {lhs_value, is_zero_undef});
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITANDLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[0]);
	auto rhs = GetValueFor(insn->operands[1]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateAnd(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITORLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[0]);
	auto rhs = GetValueFor(insn->operands[1]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateOr(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITXORLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[0]);
	auto rhs = GetValueFor(insn->operands[1]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateXor(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}


bool BlockJITSHLLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateShl(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITSHRLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto max_shift = ::llvm::ConstantInt::get(lhs->getType(), insn->operands[0].size*8, false);
	auto comparison = GetBuilder().CreateICmpUGE(rhs, max_shift);
	auto shift = GetBuilder().CreateLShr(lhs, rhs);
	auto selected_shift = GetBuilder().CreateSelect(comparison, llvm::ConstantInt::get(lhs->getType(), 0), shift);
	
	SetValueFor(insn->operands[1], selected_shift);
		
	insn++;
	
	return true;
}

bool BlockJITSARLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto max_shift = ::llvm::ConstantInt::get(lhs->getType(), insn->operands[0].size*8, false);
	auto comparison = GetBuilder().CreateICmpUGE(rhs, max_shift);
	auto shift = GetBuilder().CreateAShr(lhs, rhs);
	auto selected_shift = GetBuilder().CreateSelect(comparison, llvm::ConstantInt::get(lhs->getType(), 0), shift);
	
	SetValueFor(insn->operands[1], selected_shift);
		
	insn++;
	
	return true;
}
