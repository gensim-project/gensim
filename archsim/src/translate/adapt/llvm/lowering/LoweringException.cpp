/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITEXCEPTIONLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &category = insn->operands[0];
	const auto &data = insn->operands[1];
	
	// create call to cpuTakeException
	GetBuilder().CreateCall(GetContext().GetValues().cpuTakeExceptionPtr, {GetThreadPtr(), GetValueFor(category), GetValueFor(data)});
	
	insn++;
	
	return true;
}
