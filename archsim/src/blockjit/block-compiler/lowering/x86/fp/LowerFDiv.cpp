#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerFDiv::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &op2 = insn->operands[1];
	const IROperand &dest = insn->operands[2];

	int width = op1.size;

	assert(dest.is_vreg());

	assert(op1.size == op2.size && op2.size == dest.size);
	assert(op1.size == 4 || op1.size == 8);

	if(op1.is_alloc_reg()) {
		Encoder().movq(GetLoweringContext().register_from_operand(&op1), BLKJIT_FP_1);
	} else if(op1.is_constant()) {
		// XXX this could be optimised
		Encoder().mov(op1.value, BLKJIT_TEMPS_0(op1.size));
		Encoder().movq(BLKJIT_TEMPS_0(op1.size), BLKJIT_FP_1);
	} else {
		assert(false);
	}

	if(op2.is_alloc_reg()) {
		Encoder().movq(GetLoweringContext().register_from_operand(&op2), BLKJIT_FP_0);
	} else if(op2.is_constant()) {
		Encoder().mov(op2.value, BLKJIT_TEMPS_0(op2.size));
		Encoder().movq(BLKJIT_TEMPS_0(op2.size), BLKJIT_FP_0);
	} else {
		assert(false);
	}


	// dest = op1 * op2
	if(width == 4) {
		Encoder().divss(BLKJIT_FP_0, BLKJIT_FP_1);

		Encoder().movq(BLKJIT_FP_1, GetLoweringContext().register_from_operand(&dest));
	} else if(width == 8) {
		Encoder().divsd(BLKJIT_FP_0, BLKJIT_FP_1);

		Encoder().movq(BLKJIT_FP_1, GetLoweringContext().register_from_operand(&dest));
	}

	insn++;

	return true;
}