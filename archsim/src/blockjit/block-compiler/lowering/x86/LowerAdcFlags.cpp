/*
 * LowerAdcFlags.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerAdcFlags::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *lhs = &insn->operands[0];
	const IROperand *rhs = &insn->operands[1];
	const IROperand *carry_in = &insn->operands[2];

	auto& tmp = GetLoweringContext().get_temp(0, 4);

	bool just_do_an_add = false;

	// Set up carry bit for adc
	if (carry_in->is_constant()) {
		if (carry_in->value == 0) {
			just_do_an_add = true;
		} else {
			Encoder().stc();
		}
	} else if (carry_in->is_vreg()) {
		if (carry_in->is_alloc_reg()) {
			Encoder().mov(0xff, GetLoweringContext().get_temp(0, 1));
			Encoder().add(GetLoweringContext().register_from_operand(carry_in, 1), GetLoweringContext().get_temp(0, 1));
		} else {
			Encoder().mov(0xff, GetLoweringContext().get_temp(0, 1));
			Encoder().add(GetLoweringContext().stack_from_operand(carry_in), GetLoweringContext().get_temp(0, 1));
		}
	} else {
		assert(false);
	}

	// Load up lhs
	if (lhs->is_constant()) {
		Encoder().mov(lhs->value, tmp);
	} else if (lhs->is_vreg()) {
		if (lhs->is_alloc_reg()) {
			Encoder().mov(GetLoweringContext().register_from_operand(lhs, 4), tmp);
		} else if (lhs->is_alloc_stack()) {
			Encoder().mov(GetLoweringContext().stack_from_operand(lhs), tmp);
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}

	// Perform add with carry to set the flags
	if (rhs->is_constant()) {
		if (just_do_an_add) {
			Encoder().add((uint32_t)rhs->value, tmp);
		} else {
			Encoder().adc((uint32_t)rhs->value, tmp);
		}
	} else if (rhs->is_vreg()) {
		if (rhs->is_alloc_reg()) {
			if (just_do_an_add) {
				Encoder().add(GetLoweringContext().register_from_operand(rhs, 4), tmp);
			} else {
				Encoder().adc(GetLoweringContext().register_from_operand(rhs, 4), tmp);
			}
		} else if (rhs->is_alloc_stack()) {
			if (just_do_an_add) {
				Encoder().add(GetLoweringContext().stack_from_operand(rhs), tmp);
			} else {
				Encoder().adc(GetLoweringContext().stack_from_operand(rhs), tmp);
			}
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}

	// LOL HAX XXX
	const archsim::ArchDescriptor &arch = GetLoweringContext().GetArchDescriptor();
	auto &rfd = arch.GetRegisterFileDescriptor();
	uint32_t c_o = rfd.GetTaggedEntry("C").GetOffset();
	uint32_t v_o = rfd.GetTaggedEntry("V").GetOffset();
	uint32_t z_o = rfd.GetTaggedEntry("Z").GetOffset();
	uint32_t n_o = rfd.GetTaggedEntry("N").GetOffset();

	Encoder().setc(X86Memory::get(BLKJIT_REGSTATE_REG, c_o));
	Encoder().seto(X86Memory::get(BLKJIT_REGSTATE_REG, v_o));
	Encoder().setz(X86Memory::get(BLKJIT_REGSTATE_REG, z_o));
	Encoder().sets(X86Memory::get(BLKJIT_REGSTATE_REG, n_o));


	insn++;
	return true;
}


