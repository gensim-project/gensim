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
#include "blockjit/blockjit-abi.h"

#include "gensim/gensim_processor_state.h"
#include "util/SimOptions.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerRet::Lower(const captive::shared::IRInstruction *&insn)
{
	uint32_t max_stack = GetLoweringContext().GetStackFrameSize();
	
	Encoder().pop(BLKJIT_REGSTATE_REG);
	Encoder().pop(BLKJIT_CPUSTATE_REG);
	Encoder().mov(REG_RBP, REG_RSP);
	Encoder().pop(REG_RBP);
	
	Encoder().xorr(REG_EAX, REG_EAX);
	Encoder().ret();

	insn++;
	return true;
}
