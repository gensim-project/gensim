/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITZXLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &source = insn->operands[0];
	const auto &dest = insn->operands[1];
	
	auto value = GetValueFor(source);
	
	auto extended = GetBuilder().CreateZExt(value, GetContext().GetLLVMType(dest));
	SetValueFor(dest, extended);
	
	insn++;
	
	return true;
}
