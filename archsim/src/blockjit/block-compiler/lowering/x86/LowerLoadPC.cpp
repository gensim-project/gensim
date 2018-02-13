/*
 * LowerLoadPC.cpp
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

bool LowerLoadPC::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *target = &insn->operands[0];

	assert(target->is_vreg());

	const gensim::BlockJitProcessor *cpu = GetCompiler().get_cpu();
	uint32_t pc_offset = cpu->GetRegisterByTag("PC")->Offset;

	if (target->is_alloc_reg()) {
		// TODO: FIXME: XXX: HACK HACK HACK
		Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, pc_offset), GetCompiler().register_from_operand(target));
	} else if(target->is_alloc_stack()) {
		Encoder().mov(X86Memory::get(BLKJIT_REGSTATE_REG, pc_offset), BLKJIT_TEMPS_0(4));
		Encoder().mov(BLKJIT_TEMPS_0(4), GetCompiler().stack_from_operand(target));
	}

	insn++;
	return true;
}
