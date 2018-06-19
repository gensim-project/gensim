/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"

using namespace archsim::translate::adapt;

bool BlockJITNOPLowering::Lower(const captive::shared::IRInstruction*& insn) {
	
	insn++;
	
	return true;
}
