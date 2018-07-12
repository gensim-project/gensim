/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITVCMPILowering::Lower(const captive::shared::IRInstruction*& insn)
{
	auto size = insn->operands[0];
	auto lhs = insn->operands[1];
	auto rhs = insn->operands[2];
	auto output = insn->operands[3];

	auto vector_type = GetContext().GetVectorType(lhs, size);
	auto lhs_value = GetBuilder().CreateBitCast(GetValueFor(lhs), vector_type);
	auto rhs_value = GetBuilder().CreateBitCast(GetValueFor(lhs), vector_type);

	llvm::Value *value;
	switch(GetCmpType()) {
			using captive::shared::IRInstruction;
		case IRInstruction::VCMPEQI:
			value = GetBuilder().CreateICmpEQ(lhs_value, rhs_value);
			break;
		default:
			UNIMPLEMENTED;
	}

	value = GetBuilder().CreateSExt(value, vector_type);

	value = GetBuilder().CreateBitCast(value, GetContext().GetLLVMType(output));
	SetValueFor(output, value);

	insn++;
	return true;
}