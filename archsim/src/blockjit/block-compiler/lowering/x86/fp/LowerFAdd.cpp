/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerFAdd::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &op2 = insn->operands[1];
	const IROperand &dest = insn->operands[2];

	int width = op1.size;

	assert(dest.is_vreg());

	assert(op1.size == op2.size && op2.size == dest.size);
	assert(op1.size == 4 || op1.size == 8);

	const auto &op1_reg = op1.is_alloc_reg() ? GetLoweringContext().register_from_operand(&op1) : BLKJIT_TEMPS_0(op1.size);
	const auto &op2_reg = op2.is_alloc_reg() ? GetLoweringContext().register_from_operand(&op2) : BLKJIT_TEMPS_1(op2.size);
	const auto &dest_reg = dest.is_alloc_reg() ? GetLoweringContext().register_from_operand(&dest) : BLKJIT_TEMPS_0(dest.size);

	if(op1.is_alloc_stack()) {
		Encoder().mov(GetLoweringContext().stack_from_operand(&op1), op1_reg);
	} else if(op1.is_constant()) {
		assert(false);
	}

	if(op2.is_alloc_stack()) {
		Encoder().mov(GetLoweringContext().stack_from_operand(&op2), op2_reg);
	} else if(op2.is_constant()) {
		Encoder().mov(op2.value, BLKJIT_TEMPS_0(8));
	}

	// dest = op1 * op2
	if(width == 4) {
		Encoder().movq(op1_reg, BLKJIT_FP_1);
		Encoder().movq(op2_reg, BLKJIT_FP_0);

		Encoder().addss(BLKJIT_FP_1, BLKJIT_FP_0);

		Encoder().movq(BLKJIT_FP_0, dest_reg);
	} else if(width == 8) {
		Encoder().movq(op1_reg, BLKJIT_FP_1);
		Encoder().movq(op2_reg, BLKJIT_FP_0);

		Encoder().addsd(BLKJIT_FP_1, BLKJIT_FP_0);

		Encoder().movq(BLKJIT_FP_0, dest_reg);
	}

	if(dest.is_alloc_stack()) {
		Encoder().mov(dest_reg, GetLoweringContext().stack_from_operand(&dest));
	}

	insn++;

	return true;
}
