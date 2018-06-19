/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITCMPLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &lhs = insn->operands[0];
	const auto &rhs = insn->operands[1];
	const auto &dest = insn->operands[2];
	
	auto lhs_value = GetValueFor(lhs);
	auto rhs_value = GetValueFor(rhs);
	llvm::Value *result;
	
	switch(GetCmpType()) {
		case captive::shared::IRInstruction::CMPEQ: result = GetBuilder().CreateICmpEQ(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPNE: result = GetBuilder().CreateICmpNE(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPLT: result = GetBuilder().CreateICmpULT(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPLTE: result = GetBuilder().CreateICmpULE(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPGT: result = GetBuilder().CreateICmpUGT(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPGTE: result = GetBuilder().CreateICmpUGE(lhs_value, rhs_value); break;
		
		case captive::shared::IRInstruction::CMPSGT: result = GetBuilder().CreateICmpSGT(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPSGTE: result = GetBuilder().CreateICmpSGE(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPSLT: result = GetBuilder().CreateICmpSLT(lhs_value, rhs_value); break;
		case captive::shared::IRInstruction::CMPSLTE: result = GetBuilder().CreateICmpSLE(lhs_value, rhs_value); break;
		
		default:
			UNIMPLEMENTED;
	}
	
	result = GetBuilder().CreateZExt(result, GetContext().GetLLVMType(dest));
	SetValueFor(dest, result);
	
	insn++;
	
	return true;
}
