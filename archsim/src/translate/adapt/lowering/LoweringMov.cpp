/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"

using namespace archsim::translate::adapt;

bool BlockJITMOVLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	const auto &from = insn->operands[0];
	const auto &to = insn->operands[1];
	
	SetValueFor(to, GetValueFor(from));
	
	insn++;
	
	return true;
}
