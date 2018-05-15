/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITLDREGLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &offset = insn->operands[0];
	const auto &target = insn->operands[1];

	assert(target.is_vreg());

	assert(offset.is_constant());
	
	GetContext().SetValueFor(target, GetBuilder().CreateLoad(GetContext().GetRegisterPointer(offset.value, target.size)));
	
	insn++;
	
	return true;
}

bool BlockJITSTREGLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &offset = insn->operands[1];
	const auto &value = insn->operands[0];
	
	assert(offset.is_constant());
	
	GetBuilder().CreateStore(GetValueFor(value), GetContext().GetRegisterPointer(offset.value, value.size));
	
	insn++;
	
	return true;
}
