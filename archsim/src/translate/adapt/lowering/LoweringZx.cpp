/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
