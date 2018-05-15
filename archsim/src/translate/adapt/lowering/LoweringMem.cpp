/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"

using namespace archsim::translate::adapt;

bool BlockJITLDMEMLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	insn++;
	
	return true;
}

bool BlockJITSTMEMLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	insn++;
	
	return true;
}
