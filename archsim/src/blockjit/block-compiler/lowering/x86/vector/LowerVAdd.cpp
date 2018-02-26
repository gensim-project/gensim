
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"
#include "gensim/gensim_processor_blockjit.h"
#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerVAddI::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &width = insn->operands[0]; // VECTOR width, i.e. number of elements
	const IROperand &lhs = insn->operands[1];
	const IROperand &rhs = insn->operands[2];
	const IROperand &dest = insn->operands[3];

	//TODO: make stack based accesses more efficient (movq from memory)
	if(lhs.is_alloc_reg())
		Encoder().movq(GetCompiler().register_from_operand(&lhs), BLKJIT_FP_0);
	else {
		Encoder().mov(GetCompiler().stack_from_operand(&lhs), BLKJIT_TEMPS_0(lhs.size));
		Encoder().movq(BLKJIT_TEMPS_0(lhs.size), BLKJIT_FP_0);
	}

	if(rhs.is_alloc_reg())
		Encoder().movq(GetCompiler().register_from_operand(&rhs), BLKJIT_FP_1);
	else {
		Encoder().mov(GetCompiler().stack_from_operand(&rhs), BLKJIT_TEMPS_0(rhs.size));
		Encoder().movq(BLKJIT_TEMPS_0(lhs.size), BLKJIT_FP_1);
	}


	// emit instruction based on ELEMENT size (total vector size / number of elements)
	switch(lhs.size / width.value) {
		case 2:
			Encoder().paddw(BLKJIT_FP_0, BLKJIT_FP_1);
			break;
		case 4:
			Encoder().paddd(BLKJIT_FP_0, BLKJIT_FP_1);
			break;
		case 8:
			Encoder().paddq(BLKJIT_FP_0, BLKJIT_FP_1);
			break;
		default:
			LC_ERROR(LogBlockJit) << "Cannot lower an instruction with vector size " << (uint32_t)lhs.size << " and element count " << (uint32_t)width.value;
			assert(false);
	}

	if(dest.is_alloc_reg())
		Encoder().movq(BLKJIT_FP_1, GetCompiler().register_from_operand(&dest));
	else  {
		Encoder().movq(BLKJIT_FP_1, BLKJIT_TEMPS_0(dest.size));
		Encoder().mov(BLKJIT_TEMPS_0(dest.size), GetCompiler().stack_from_operand(&dest));
	}

	insn++;
	return true;
}

bool LowerVAddF::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &width = insn->operands[0]; // VECTOR width, i.e. number of elements
	const IROperand &lhs = insn->operands[1];
	const IROperand &rhs = insn->operands[2];
	const IROperand &dest = insn->operands[3];

	if(lhs.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&lhs), BLKJIT_FP_0);
	} else if(lhs.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&lhs), BLKJIT_TEMPS_0(lhs.size));
		Encoder().movq(BLKJIT_TEMPS_0(lhs.size), BLKJIT_FP_0);
	} else {
		assert(false);
	}
	if(rhs.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&rhs), BLKJIT_FP_1);
	} else if(rhs.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&rhs), BLKJIT_TEMPS_0(rhs.size));
		Encoder().movq(BLKJIT_TEMPS_0(rhs.size), BLKJIT_FP_1);
	} else {
		assert(false);
	}

	// emit instruction based on ELEMENT size (total vector size / number of elements)
	switch(lhs.size / width.value) {
		case 4:
			Encoder().addps(BLKJIT_FP_0, BLKJIT_FP_1);
			break;
		case 8:
			Encoder().addpd(BLKJIT_FP_0, BLKJIT_FP_1);
			break;
		default:
			LC_ERROR(LogBlockJit) << "Cannot lower an instruction with vector size " << (uint32_t)lhs.size << " and element count " << (uint32_t)width.value;
			assert(false);
	}

	if(dest.is_alloc_reg()) {
		Encoder().movq(BLKJIT_FP_1, GetCompiler().register_from_operand(&dest));
	} else if(dest.is_alloc_stack()) {
		Encoder().movq(BLKJIT_FP_1, BLKJIT_TEMPS_0(dest.size));
		Encoder().mov(BLKJIT_TEMPS_0(dest.size), GetCompiler().stack_from_operand(&dest));
	} else {
		assert(false);
	}

	insn++;
	return true;
}