/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
