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
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerIncPC::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *amount = &insn->operands[0];
	const auto &rfd = GetLoweringContext().GetArchDescriptor().GetRegisterFileDescriptor();
	uint32_t pc_offset = rfd.GetTaggedEntry("PC").GetOffset();

	if (amount->is_constant()) {
		Encoder().add4(amount->value, X86Memory::get(BLKJIT_REGSTATE_REG, pc_offset));
	} else {
		assert(false);
	}

	insn++;
	return true;
}


