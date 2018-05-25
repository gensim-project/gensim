/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
