/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITFCNTL_SETROUNDLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &mode = insn->operands[0];
	
	GetBuilder().CreateCall(GetContext().GetValues().cpuSetRoundingModePtr, {GetThreadPtr(), GetValueFor(mode)});
	
	insn++;
	
	return true;
}

bool BlockJITFCNTL_GETROUNDLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &output = insn->operands[0];
	
	auto result = GetBuilder().CreateCall(GetContext().GetValues().cpuGetRoundingModePtr, {GetThreadPtr()});
	SetValueFor(output, result);
	
	insn++;
	
	return true;
}

bool BlockJITFCNTL_SETFLUSHLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &mode = insn->operands[0];
	
	GetBuilder().CreateCall(GetContext().GetValues().cpuSetFlushModePtr, {GetThreadPtr(), GetValueFor(mode)});
	
	insn++;
	
	return true;
}

bool BlockJITFCNTL_GETFLUSHLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &output = insn->operands[0];
	
	auto result = GetBuilder().CreateCall(GetContext().GetValues().cpuGetFlushModePtr, {GetThreadPtr()});
	SetValueFor(output, result);
	
	insn++;
	
	return true;
}
