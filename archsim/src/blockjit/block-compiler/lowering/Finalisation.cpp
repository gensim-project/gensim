/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * Finalisation.cpp
 *
 *  Created on: 1 Dec 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/Finalisation.h"
#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"

using namespace captive::arch::jit::lowering;

Finalisation::~Finalisation()
{

}

bool X86Finalisation::Finalise(LoweringContext& context) {
	return FinaliseX86(static_cast<x86::X86LoweringContext&>(context));
}
