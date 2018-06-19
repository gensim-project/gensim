/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LowerZNFlags.cpp
 *
 *  Created on: 1 Dec 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerZNFlags::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand &value = insn->operands[0];

	const auto &rfd = GetLoweringContext().GetArchDescriptor().GetRegisterFileDescriptor();
	uint32_t z_o = rfd.GetTaggedEntry("Z").GetOffset();
	uint32_t n_o = rfd.GetTaggedEntry("N").GetOffset();
	
	if(value.is_alloc_reg()) {
		auto &input_reg = GetLoweringContext().register_from_operand(&value);
		Encoder().test(input_reg, input_reg);

		Encoder().setz(X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
		Encoder().sets(X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
	} else if(value.is_alloc_stack()) {
		assert(false);
	} else if(value.is_constant()) {
		// generate instructions depending on fixed value
		// 3 different conditions (since value cannot be signed and zero)
		int64_t v = value.value;

		if(v == 0) {
			// clear n, set Z
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
			Encoder().mov1(1, X86Memory::get(BLKJIT_REGSTATE_REG, z_o));

		} else if(v < 0) {
			// set n, clear Z
			Encoder().mov1(1, X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
		} else {
			// clear n and z
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, n_o));
			Encoder().mov1(0, X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
		}

	} else {
		assert(false);
	}

	insn++;
	return true;
}


