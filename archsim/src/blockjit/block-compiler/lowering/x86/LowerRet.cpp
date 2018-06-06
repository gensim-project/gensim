/*
 * LowerRet.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "util/SimOptions.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerRet::Lower(const captive::shared::IRInstruction *&insn)
{	
	GetLoweringContext().EmitEpilogue();
	Encoder().ret();

	insn++;
	return true;
}
