/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;


bool BlockJITADCFLAGSLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[0]);
	auto rhs = GetValueFor(insn->operands[1]);
	auto carry = GetValueFor(insn->operands[2]);
	
	lhs = GetBuilder().CreateZExtOrTrunc(lhs, GetContext().GetLLVMType(4));
	rhs = GetBuilder().CreateZExtOrTrunc(rhs, GetContext().GetLLVMType(4));
	carry = GetBuilder().CreateZExtOrTrunc(carry, GetContext().GetLLVMType(1));
	
	llvm::Value *packedword = GetBuilder().CreateCall(GetContext().GetValues().genc_adc_flags_ptr, {lhs, rhs, carry});
	
	// unpack c, n, v, z
	auto c_value = GetBuilder().CreateLShr(packedword, 8);
	c_value = GetBuilder().CreateAnd(c_value, 1);
	c_value = GetBuilder().CreateZExtOrTrunc(c_value, GetContext().GetLLVMType(1));
	
	auto n_value = GetBuilder().CreateLShr(packedword, 15);
	n_value = GetBuilder().CreateAnd(n_value, 1);
	n_value = GetBuilder().CreateZExtOrTrunc(n_value, GetContext().GetLLVMType(1));
	
	auto v_value = packedword;
	v_value = GetBuilder().CreateAnd(v_value, 1);
	v_value = GetBuilder().CreateZExtOrTrunc(v_value, GetContext().GetLLVMType(1));
	
	auto z_value = GetBuilder().CreateLShr(packedword, 14);
	z_value = GetBuilder().CreateAnd(z_value, 1);
	z_value = GetBuilder().CreateZExtOrTrunc(z_value, GetContext().GetLLVMType(1));
	
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
