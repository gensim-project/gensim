/*
 * LowerJmp.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerJmp::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *target = &insn->operands[0];
	assert(target->is_block());

	const IRInstruction *next_insn = insn+1;

	if (next_insn && next_insn->ir_block == (IRBlockId)target->value) {
		// The next instruction is in the block we're about to jump to, so we can
		// leave this as a fallthrough.
	} else {
		// Create a relocated jump instruction, and store the relocation offset and
		// target into the relocations list.
		uint32_t reloc_offset;
		Encoder().jmp_reloc(reloc_offset);

		GetLoweringContext().RegisterBlockRelocation(reloc_offset, target->value);

		Encoder().align_up(8);
	}

	insn++;
	return true;
}
