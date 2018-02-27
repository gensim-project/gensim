
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

bool LowerVMulI::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &width = insn->operands[0]; // VECTOR width, i.e. number of elements
	const IROperand &lhs = insn->operands[1];
	const IROperand &rhs = insn->operands[2];
	const IROperand &dest = insn->operands[3];

	// No straightforward parallel integer multiply in SSE, so for now just do two dumb multiplies
	// also assume basic 2x32 multiply for now
	assert(lhs.size == 8);
	assert(width.value == 2);

	const auto &lhsr = GetCompiler().register_from_operand(&lhs);
	const auto &rhsr = GetCompiler().register_from_operand(&rhs);
	const auto &destr = GetCompiler().register_from_operand(&dest);

	// start with top multiply
	Encoder().mov(lhsr, BLKJIT_TEMPS_0(8));
	Encoder().shr(32, BLKJIT_TEMPS_0(8));
	Encoder().mov(rhsr, destr);
	Encoder().shr(32, destr);
	Encoder().mul(BLKJIT_TEMPS_0(8), destr);
	Encoder().shl(32, destr);

	// now do bottom multiply
	Encoder().mov(GetCompiler().register_from_operand(&rhs, 4), BLKJIT_TEMPS_0(4));
	Encoder().mul(GetCompiler().register_from_operand(&lhs, 4), BLKJIT_TEMPS_0(4));
	Encoder().orr(BLKJIT_TEMPS_0(8), destr);

	insn++;
	return true;
}

bool LowerVMulF::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &width = insn->operands[0]; // VECTOR width, i.e. number of elements
	const IROperand &lhs = insn->operands[1];
	const IROperand &rhs = insn->operands[2];
	const IROperand &dest = insn->operands[3];

	if(lhs.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&lhs), BLKJIT_FP_0);
	} else  if(lhs.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&lhs), BLKJIT_TEMPS_0(lhs.size));
		Encoder().movq(BLKJIT_TEMPS_0(lhs.size), BLKJIT_FP_0);
	} else {
		assert(false);
	}
	
	if(rhs.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&rhs), BLKJIT_FP_1);
	} else  if(rhs.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&rhs), BLKJIT_TEMPS_0(rhs.size));
		Encoder().movq(BLKJIT_TEMPS_0(rhs.size), BLKJIT_FP_1);
	} else {
		assert(false);
	}
	
	// emit instruction based on ELEMENT size (total vector size / number of elements)
	switch(lhs.size / width.value) {
		case 4:
			Encoder().mulps(BLKJIT_FP_0, BLKJIT_FP_1);
			break;
		case 8:
			Encoder().mulpd(BLKJIT_FP_0, BLKJIT_FP_1);
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
	}

	insn++;
	return true;
}