/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITVORILowering::Lower(const captive::shared::IRInstruction*& insn)
{
	auto size = insn->operands[0];
	auto lhs = insn->operands[1];
	auto rhs = insn->operands[2];
	auto output = insn->operands[3];

	auto vector_type = GetContext().GetVectorType(lhs, size);
	auto lhs_value = GetValueFor(lhs);
	auto rhs_value = GetValueFor(rhs);

	auto output_value = GetBuilder().CreateOr(lhs_value, rhs_value);
	SetValueFor(output, output_value);

	insn++;
	return true;
}