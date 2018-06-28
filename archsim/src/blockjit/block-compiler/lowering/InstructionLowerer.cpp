/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * InstructionLowerer.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/InstructionLowerer.h"

using namespace captive::arch::jit::lowering;

InstructionLowerer::InstructionLowerer()
{

}

InstructionLowerer::~InstructionLowerer()
{

}

bool InstructionLowerer::TxlnStart(captive::arch::jit::BlockCompiler *compiler, captive::arch::jit::TranslationContext *ctx)
{
	return true;
}

NOPInstructionLowerer::NOPInstructionLowerer()
{

}

bool NOPInstructionLowerer::Lower(const captive::shared::IRInstruction *&insn)
{
	insn++;
	return true;
}
