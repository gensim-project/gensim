/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITLDREGLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &offset = insn->operands[0];
	const auto &target = insn->operands[1];

	assert(target.is_vreg());
	
	if(offset.is_constant()) {
		GetContext().SetValueFor(target, GetBuilder().CreateLoad(GetContext().GetRegisterPointer(offset.value, target.size)));
	} else {
		GetContext().SetValueFor(target, GetBuilder().CreateLoad(GetContext().GetRegisterPointer(GetValueFor(offset), target.size)));
	}
	
	insn++;
	
	return true;
}

bool BlockJITSTREGLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &offset = insn->operands[1];
	const auto &value = insn->operands[0];
	
	if(offset.is_constant()) {
		GetBuilder().CreateStore(GetValueFor(value), GetContext().GetRegisterPointer(offset.value, value.size));
	} else {
		GetBuilder().CreateStore(GetValueFor(value), GetContext().GetRegisterPointer(GetValueFor(offset), value.size));
	}
	
	insn++;
	
	return true;
}
