/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;


bool BlockJITADCFLAGSLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs_orig = GetValueFor(insn->operands[0]);
	auto rhs_orig = GetValueFor(insn->operands[1]);
	auto carry = GetValueFor(insn->operands[2]);
	
	auto lhs = GetBuilder().CreateZExtOrTrunc(lhs_orig, GetContext().GetLLVMType(8));
	auto rhs = GetBuilder().CreateZExtOrTrunc(rhs_orig, GetContext().GetLLVMType(8));
	carry = GetBuilder().CreateZExtOrTrunc(carry, GetContext().GetLLVMType(8));
	
	auto result = GetBuilder().CreateAdd(lhs, rhs);
	result = GetBuilder().CreateAdd(result, carry);
	auto result32 = GetBuilder().CreateTrunc(result, GetContext().GetLLVMType(4));
	
	auto n_value_orig = GetBuilder().CreateICmpSLT(result32, llvm::ConstantInt::get(GetContext().GetLLVMType(4), 0, false));
	auto n_value = GetBuilder().CreateZExtOrTrunc(n_value_orig, GetContext().GetLLVMType(1));
	
	auto z_value = GetBuilder().CreateICmpEQ(result32, llvm::ConstantInt::get(GetContext().GetLLVMType(4), 0, false));
	z_value = GetBuilder().CreateZExtOrTrunc(z_value, GetContext().GetLLVMType(1));
	
	auto c_value = GetBuilder().CreateICmpUGT(result, llvm::ConstantInt::get(result->getType(), 0xffffffff, false));
	c_value = GetBuilder().CreateZExtOrTrunc(c_value, GetContext().GetLLVMType(1));
	
	auto lhs_sign = GetBuilder().CreateICmpSLT(lhs_orig, llvm::ConstantInt::get(lhs_orig->getType(), 0, false));
	auto rhs_sign = GetBuilder().CreateICmpSLT(rhs_orig, llvm::ConstantInt::get(rhs_orig->getType(), 0, false));
	
	auto v_value = GetBuilder().CreateAnd(GetBuilder().CreateICmpEQ(lhs_sign, rhs_sign), GetBuilder().CreateICmpNE(lhs_sign, n_value_orig));
	v_value = GetBuilder().CreateZExtOrTrunc(v_value, GetContext().GetLLVMType(1));
	
	// write them back
	GetBuilder().CreateStore(c_value, GetContext().GetTaggedRegisterPointer("C"));
	GetBuilder().CreateStore(n_value, GetContext().GetTaggedRegisterPointer("N"));
	GetBuilder().CreateStore(v_value, GetContext().GetTaggedRegisterPointer("V"));
	GetBuilder().CreateStore(z_value, GetContext().GetTaggedRegisterPointer("Z"));
	
	insn++;
	
	return true;
}

bool BlockJITADDLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[0]);
	auto rhs = GetValueFor(insn->operands[1]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateAdd(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITSUBLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateSub(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITMODLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateURem(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITSDIVLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateSDiv(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}

bool BlockJITUMULLLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[1]);
	auto rhs = GetValueFor(insn->operands[0]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateMul(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}
