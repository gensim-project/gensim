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

/*** THESE FUNCTIONS USE THE BLOCKJIT ABI ***/
// ARG AND RETURN in BLOCKJIT_TEMPS_0
extern "C" uint64_t blkjit_cvtt_f_to_u64(float);
extern "C" uint64_t blkjit_cvtt_d_to_u64(double);
extern "C" uint64_t blkjit_cvt_f_to_u64(float);
extern "C" uint64_t blkjit_cvt_d_to_u64(double);

extern "C" float blkjit_cvt_u64_to_f(uint64_t);
extern "C" double blkjit_cvt_u64_to_d(uint64_t);
/*** END OF BLOCKJIT ABI FUNCTIONS ***/

bool LowerFCvt_SI_To_F::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &op1 = insn->operands[0];
	const IROperand &dest = insn->operands[1];

	assert(dest.is_vreg());
	assert(dest.is_alloc_reg());

	const auto &op_reg = GetCompiler().register_from_operand(&op1);


	if(dest.size == 4) {
		Encoder().cvtsi2ss(GetCompiler().register_from_operand(&op1, 4), BLKJIT_FP_0);
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

	auto &dest_reg = dest.is_alloc_reg() ? GetCompiler().register_from_operand(&dest) : BLKJIT_TEMPS_0(dest.size);

	if(op1.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	} else if(op1.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&op1), BLKJIT_TEMPS_1(op1.size));
		Encoder().movq(BLKJIT_TEMPS_1(op1.size), BLKJIT_FP_0);
	} else {
		assert(false);
	}

	if(insn->type == IRInstruction::FCVTT_F_TO_SI) {
		if(op1.size == 4) {
			Encoder().cvttss2si(BLKJIT_FP_0, dest_reg);
		} else if(op1.size == 8) {
			Encoder().cvttsd2si(BLKJIT_FP_0, dest_reg);
		} else {
			assert(false);
		}
	} else {
		if(op1.size == 4) {
			Encoder().cvtss2si(BLKJIT_FP_0, dest_reg);
		} else if(op1.size == 8) {
			Encoder().cvtsd2si(BLKJIT_FP_0, dest_reg);
		} else {
			assert(false);
		}
	}

	if(dest.is_alloc_stack()) {
		Encoder().mov(BLKJIT_TEMPS_0(dest.size), GetCompiler().stack_from_operand(&dest));
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

	// Convert unsigned integer to float
	// 4 cases: U32 to Float, U32 to Double, U64 to Float, U64 To Double
	if(op1.size == 4 && dest.size == 4) {
		// U32 to float
		// zero extend input register to 64 bits
		Encoder().mov(GetCompiler().register_from_operand(&op1), GetCompiler().register_from_operand(&op1));
		// convert to float
		Encoder().cvtsi2ss(GetCompiler().register_from_operand(&op1, 8), BLKJIT_FP_0);
	} else if(op1.size == 4 && dest.size == 8) {
		// U32 to double
		Encoder().pxor(BLKJIT_FP_0, BLKJIT_FP_0);
		// zero extend input register to 64 bits
		Encoder().mov(GetCompiler().register_from_operand(&op1), GetCompiler().register_from_operand(&op1));
		// convert to double
		Encoder().cvtsi2sd(GetCompiler().register_from_operand(&op1, 8), BLKJIT_FP_0);
	} else if(op1.size == 8 && dest.size == 4) {
		// U64 to float
		// requires a call
		GetCompiler().encode_operand_to_reg(&op1, BLKJIT_TEMPS_0(op1.size));
		Encoder().call((void*)blkjit_cvt_u64_to_f, BLKJIT_RETURN(8));
	} else if(op1.size == 8 && dest.size == 8) {
		// u64 to double
		// requires a call
		GetCompiler().encode_operand_to_reg(&op1, BLKJIT_TEMPS_0(op1.size));
		Encoder().call((void*)blkjit_cvt_u64_to_d, BLKJIT_RETURN(8));
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

	if(op1.is_alloc_reg()) {
		Encoder().movq(GetCompiler().register_from_operand(&op1), BLKJIT_FP_0);
	} else if(op1.is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(&op1), BLKJIT_TEMPS_0(op1.size));
		Encoder().movq(BLKJIT_TEMPS_0(op1.size), BLKJIT_FP_0);
	}

	// if dest is u32, then we can do this with one instruction
	if(dest.size == 4) {
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
	} else if(dest.size == 8) {
		// if dest is u64, then we need to encode a call to a helper
		void *fn = nullptr;

		if(insn->type == IRInstruction::FCVTT_F_TO_UI) {
			if(op1.size == 4) {
				fn = (void*)blkjit_cvtt_f_to_u64;
			} else if(op1.size == 8) {
				fn = (void*)blkjit_cvtt_d_to_u64;
			} else {
				assert(false);
			}
		} else {
			if(op1.size == 4) {
				fn = (void*)blkjit_cvt_f_to_u64;
			} else if(op1.size == 8) {
				fn = (void*)blkjit_cvt_d_to_u64;
			} else {
				assert(false);
			}
		}

		// now encode a call to the function
		// move argument into BLKJIT_TEMPS_0
		if(op1.is_alloc_reg()) {
			Encoder().mov(GetCompiler().register_from_operand(&op1), BLKJIT_TEMPS_0(op1.size));
		} else if(op1.is_alloc_stack()) {
			Encoder().mov(GetCompiler().stack_from_operand(&op1), BLKJIT_TEMPS_0(op1.size));
		} else {
			assert(false);
		}

		// make call
		Encoder().call(fn, BLKJIT_RETURN(8));

		// move returned value to destination
		if(dest.is_alloc_reg()) {
			Encoder().mov(BLKJIT_TEMPS_0(dest.size), GetCompiler().register_from_operand(&dest));
		} else if(dest.is_alloc_stack()) {
			Encoder().mov(BLKJIT_TEMPS_0(dest.size), GetCompiler().stack_from_operand(&dest));
		} else {
			assert(false);
		}

	} else {
		assert(false);
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