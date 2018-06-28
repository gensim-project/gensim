/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITSETFEATURELowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &feature  = insn->operands[0];
	const auto &value  = insn->operands[1];

	GetBuilder().CreateCall(GetContext().GetValues().cpuSetFeaturePtr, {GetThreadPtr(), GetValueFor(feature), GetValueFor(value)});

	insn++;

	return true;
}
