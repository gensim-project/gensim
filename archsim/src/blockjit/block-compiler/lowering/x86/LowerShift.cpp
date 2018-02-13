/*
 * LowerShift.cpp
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

bool ShiftInsn(const IRInstruction *insn, X86Encoder &encoder, const X86Register &operand)
{
	switch (insn->type) {
		case IRInstruction::SHL:
			encoder.shl(REG_CL, operand);
			break;
		case IRInstruction::SHR:
			encoder.shr(REG_CL, operand);
			break;
		case IRInstruction::SAR:
			encoder.sar(REG_CL, operand);
			break;
		case IRInstruction::ROR:
			encoder.ror(REG_CL, operand);
			break;
		default:
			assert(false);
			break;
	}

	return true;
}

static bool ShiftSituation1(const captive::shared::IRInstruction *insn, captive::arch::x86::X86Encoder &encoder, const X86Register &shift_amount, const X86Register &bits)
{
	encoder.mov(REG_RCX, BLKJIT_TEMPS_2(8));
	encoder.mov(shift_amount, REGS_RCX(shift_amount.size));
	ShiftInsn(insn, encoder, bits);
	encoder.mov(BLKJIT_TEMPS_2(8), REG_RCX);

	return true;
}

static bool ShiftSituation2(const captive::shared::IRInstruction *insn, captive::arch::x86::X86Encoder &encoder, const X86Register &shift_amount, const X86Register &bits)
{
	assert(shift_amount != REGS_RCX(shift_amount.size));
	assert(bits == REGS_RCX(bits.size));

	encoder.xchg(bits, shift_amount);
	ShiftInsn(insn, encoder, shift_amount);
	encoder.xchg(bits, shift_amount);

	return true;
}

static bool ShiftSituation3(const captive::shared::IRInstruction *insn, captive::arch::x86::X86Encoder &encoder, const X86Register &shift_amount, const X86Register &bits)
{
	assert(shift_amount == REGS_RCX(shift_amount.size));
	assert(bits != REGS_RCX(bits.size));

	ShiftInsn(insn, encoder, bits);

	return true;
}

bool LowerShift::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *amount = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	if (amount->is_constant()) {
		if (dest->is_alloc_reg()) {
			auto& operand = GetCompiler().register_from_operand(dest);

			switch (insn->type) {
				case IRInstruction::SHL:
					Encoder().shl(amount->value, operand);
					break;
				case IRInstruction::SHR:
					Encoder().shr(amount->value, operand);
					break;
				case IRInstruction::SAR:
					Encoder().sar(amount->value, operand);
					break;
				case IRInstruction::ROR:
					Encoder().ror(amount->value, operand);
					break;
				default:
					assert(false);
					break;
			}
		} else {
			auto operand = GetCompiler().stack_from_operand(dest);

			switch (insn->type) {
				case IRInstruction::SHL:
					Encoder().shl(amount->value, dest->size, operand);
					break;
				case IRInstruction::SHR:
					Encoder().shr(amount->value, dest->size, operand);
					break;
				case IRInstruction::SAR:
					Encoder().sar(amount->value, dest->size, operand);
					break;
				case IRInstruction::ROR:
					Encoder().ror(amount->value, dest->size, operand);
					break;
				default:
					assert(false);
					break;
			}
		}
	} else if (amount->is_vreg()) {
		// this gets complicated since we have to have the shift amoutn in CL
		// which might be a) allocated and b) the value we are trying to shift.

		// We have three situations (RAX and RBX could be any register):
		// 1. Amount is 'RAX', bits is 'RBX'
		// 2. Amount is 'RAX', bits is 'RCX'
		// 3. Amount is 'RCX', bits is 'RAX'

		const X86Register *amount_reg, *bits_reg;

		if(amount->is_alloc_stack()) {
			amount_reg = &BLKJIT_TEMPS_0(amount->size);
			Encoder().mov(GetCompiler().stack_from_operand(amount), *amount_reg);
		} else {
			amount_reg = &GetCompiler().register_from_operand(amount);
		}
		if(dest->is_alloc_stack()) {
			bits_reg = &BLKJIT_TEMPS_1(amount->size);
			Encoder().mov(GetCompiler().stack_from_operand(dest), *bits_reg);
		} else {
			bits_reg = &GetCompiler().register_from_operand(dest);
		}

		if(amount_reg != &REGS_RCX(amount->size) && bits_reg != &REGS_RCX(dest->size)) {
			ShiftSituation1(insn, Encoder(), *amount_reg, *bits_reg);
		} else if(amount_reg != &REGS_RCX(amount->size) && bits_reg == &REGS_RCX(dest->size)) {
			ShiftSituation2(insn, Encoder(), *amount_reg, *bits_reg);
		} else if(amount_reg == &REGS_RCX(amount->size) && bits_reg != &REGS_RCX(dest->size)) {
			ShiftSituation3(insn, Encoder(), *amount_reg, *bits_reg);
		} else {
			// something unexpected happened
			assert(false);
		}

		if(dest->is_alloc_stack()) {
			Encoder().mov(*bits_reg, GetCompiler().stack_from_operand(dest));
		}

	} else {
		assert(false);
	}

	insn++;
	return true;
}


