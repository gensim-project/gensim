#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerFCvt_SI_To_F::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	const auto &op_reg = GetCompiler().register_from_operand(&op1);


	if(dest.size == 4) {
		// Always move to wide integer register (8 byte value is then truncated into whatever fits into the destination))
		Encoder().cvtsi2ss(GetCompiler().register_from_operand(&op1, 8), BLKJIT_FP_0);
	} else if(dest.size == 8) {
		if(op1.size == 4) {
			Encoder().movsx(GetCompiler().register_from_operand(&op1, 4), GetCompiler().register_from_operand(&dest, 8));
			Encoder().cvtsi2sd(GetCompiler().register_from_operand(&dest, 8), BLKJIT_FP_0);
		} else {
			Encoder().cvtsi2sd(GetCompiler().register_from_operand(&op1, 8), BLKJIT_FP_0);
		}
	} else {
		assert(false);
	}

	Encoder().movq(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));

	insn++;
	return true;
}

bool LowerFCvt_F_To_SI::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);

	if(insn->type == IRInstruction::FCVTT_F_TO_SI) {
		if(op1.size == 4) {
			Encoder().cvttss2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));
		} else if(op1.size == 8) {
			Encoder().cvttsd2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));
		} else {
			assert(false);
		}
	} else {
		if(op1.size == 4) {
			Encoder().cvtss2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));
		} else if(op1.size == 8) {
			Encoder().cvtsd2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));
		} else {
			assert(false);
		}
	}

	insn++;
	return true;
}

bool LowerFCvt_UI_To_F::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	Encoder().pxor(BLKJIT_FP_0, BLKJIT_FP_0);

	if(op1.size == 4) {
		Encoder().mov(GetCompiler().register_from_operand(&op1), GetCompiler().register_from_operand(&op1));
	}

	if(dest.size == 4) {
		Encoder().cvtsi2ss(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	} else if(dest.size == 8) {
		if(op1.size == 4) Encoder().mov(GetCompiler().register_from_operand(&op1), GetCompiler().register_from_operand(&op1));
		Encoder().cvtsi2sd(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	} else {
		assert(false);
	}

	Encoder().movq(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));

	insn++;
	return true;
}

bool LowerFCvt_F_To_UI::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);

	if(insn->type == IRInstruction::FCVTT_F_TO_UI) {
		if(op1.size == 4) {
			// Always move to wide integer register (8 byte value is then truncated into whatever fits into the destination))
			Encoder().cvttss2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest, 8));
		} else if(op1.size == 8) {
			Encoder().cvttsd2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest, 8));
		} else {
			assert(false);
		}
	} else {
		if(op1.size == 4) {
			// Always move to wide integer register (8 byte value is then truncated into whatever fits into the destination))
			Encoder().cvtss2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest, 8));
		} else if(op1.size == 8) {
			Encoder().cvtsd2si(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest, 8));
		} else {
			assert(false);
		}
	}

	insn++;
	return true;
}

bool LowerFCvt_S_To_D::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	Encoder().cvtss2sd(BLKJIT_FP_0, BLKJIT_FP_0);
	Encoder().movq(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));

	insn++;
	return true;
}

bool LowerFCvt_D_To_S::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	Encoder().cvtsd2ss(BLKJIT_FP_0, BLKJIT_FP_0);
	Encoder().movq(BLKJIT_FP_0, GetCompiler().register_from_operand(&dest));

	insn++;
	return true;
}