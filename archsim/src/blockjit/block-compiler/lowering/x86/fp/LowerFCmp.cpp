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

bool LowerFCmp::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &op2 = insn->operands[1];
	const IROperand &dest = insn->operands[2];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	assert(op1.size == op2.size);
	assert(dest.size == 1);
	assert(op1.size == 4 || op1.size == 8);

	// Trying to encode op1 OP op2

	// Move operands into registers
	if(op1.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	} else if(op1.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&op1), BLKJIT_TEMPS_0(op1.size));
		Encoder().movq(BLKJIT_TEMPS_0(op1.size), BLKJIT_FP_0);
	} else {
		assert(false);
	}

	if(op2.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&op2), BLKJIT_FP_1);
	} else if(op2.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&op2), BLKJIT_TEMPS_0(op2.size));
		Encoder().movq(BLKJIT_TEMPS_0(op2.size), BLKJIT_FP_1);
	} else if(op2.is_constant()) {
		Encoder().mov(op2.value, BLKJIT_TEMPS_0(op2.size));
		Encoder().movq(BLKJIT_TEMPS_0(op2.size), BLKJIT_FP_1);
	} else {
		assert(false);
	}


	uint8_t swap = 0;
	uint8_t cmp_imm = 0;

	// Encode comparison
	switch(insn->type) {
		case IRInstruction::FCMP_LT:
			cmp_imm = 1;
			break;
		case IRInstruction::FCMP_LTE:
			cmp_imm = 2;
			break;
		case IRInstruction::FCMP_GT:
			cmp_imm = 2;
			swap = 1;
			break;
		case IRInstruction::FCMP_GTE:
			cmp_imm = 1;
			swap = 1;
			break;
		case IRInstruction::FCMP_EQ:
			cmp_imm = 0;
			break;
		case IRInstruction::FCMP_NE:
			cmp_imm = 4;
			break;

		default:
			assert(false && "Unimplemented");
	}

	// cmpss(src, dest)
	// operation is dest OP src

	// by default, we need dest = op1, src = op2
	// therefore dest = fp_0, src = fp_1

	if(op1.size == 4) {
		if(swap)
			Encoder().cmpss(BLKJIT_FP_0, BLKJIT_FP_1, cmp_imm);
		else
			Encoder().cmpss(BLKJIT_FP_1, BLKJIT_FP_0, cmp_imm);
	} else if(op1.size == 8) {
		if(swap)
			Encoder().cmpsd(BLKJIT_FP_0, BLKJIT_FP_1, cmp_imm);
		else
			Encoder().cmpsd(BLKJIT_FP_1, BLKJIT_FP_0, cmp_imm);
	} else {
		assert(false && "Unimplemented");
	}

	// Move result into dest
	if(swap) {
		Encoder().movq(BLKJIT_FP_1, BLKJIT_TEMPS_0(4));
	} else {
		Encoder().movq(BLKJIT_FP_0, BLKJIT_TEMPS_0(4));
	}

	// Mask out destination
	Encoder().andd(1, BLKJIT_TEMPS_0(4));
	Encoder().mov(BLKJIT_TEMPS_0(dest.size), GetCompiler().register_from_operand(&dest));

	insn++;

	return true;
}