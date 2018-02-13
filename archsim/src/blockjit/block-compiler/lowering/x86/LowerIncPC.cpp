/*
 * LowerIncPC.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"
#include "gensim/gensim_processor_blockjit.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerIncPC::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *amount = &insn->operands[0];
	const gensim::BlockJitProcessor *cpu = GetCompiler().get_cpu();
	uint32_t pc_offset = cpu->GetRegisterByTag("PC")->Offset;

	if (amount->is_constant()) {
		Encoder().add4(amount->value, X86Memory::get(BLKJIT_REGSTATE_REG, pc_offset));
	} else {
		assert(false);
	}

	insn++;
	return true;
}


