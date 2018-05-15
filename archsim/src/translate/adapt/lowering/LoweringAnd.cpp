/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"

using namespace archsim::translate::adapt;

bool BlockJITANDLowering::Lower(const captive::shared::IRInstruction*& insn) {
	auto lhs = GetValueFor(insn->operands[0]);
	auto rhs = GetValueFor(insn->operands[1]);
	
	auto &dest = insn->operands[1];
	
	auto value = GetBuilder().CreateAnd(lhs, rhs);
	
	SetValueFor(dest, value);
	
	insn++;
	
	return true;
}
