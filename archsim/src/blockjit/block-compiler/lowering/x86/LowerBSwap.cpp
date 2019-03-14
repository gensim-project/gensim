/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerBSwap::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto operand = insn->operands[0];
	const auto destination = insn->operands[1];

	// load value into reg
	auto &dest_reg = destination.is_alloc_reg() ? GetLoweringContext().register_from_operand(&destination) : BLKJIT_TEMPS_0(destination.size);

	if(operand.is_alloc_reg()) {
		Encoder().mov(GetLoweringContext().register_from_operand(&operand), dest_reg);
	} else if(operand.is_alloc_stack()) {
		Encoder().mov(GetLoweringContext().stack_from_operand(&operand), dest_reg);
	} else {
		UNIMPLEMENTED;
	}

	Encoder().bswap(dest_reg);

	if(destination.is_alloc_stack()) {
		Encoder().mov(dest_reg, GetLoweringContext().stack_from_operand(&destination));
	}

	insn++;
	return true;
}