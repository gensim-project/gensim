/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITSETFEATURELowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &feature  = insn->operands[0];
	const auto &value  = insn->operands[1];
	
	GetBuilder().CreateCall(GetContext().GetValues().cpuSetFeaturePtr, {GetThreadPtr(), GetValueFor(feature), GetValueFor(value)});
	
	insn++;
	
	return true;
}
