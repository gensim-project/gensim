/*
 * LowerMul.cpp
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

bool LowerIMul::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	auto &dest_reg = dest->is_alloc_reg() ? GetCompiler().register_from_operand(dest) : BLKJIT_TEMPS_1(source->size);

	assert(dest->is_vreg());

	if(dest->is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(dest), dest_reg);
	}

	if(source->is_constant()) {
		Encoder().mov(source->value, BLKJIT_TEMPS_0(source->size));
		Encoder().imul(BLKJIT_TEMPS_0(source->size), dest_reg);
	} else if(source->is_alloc_reg()) {
		Encoder().imul(GetCompiler().register_from_operand(source), dest_reg);
	} else if(source->is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(source), BLKJIT_TEMPS_0(source->size));
		
		Encoder().imul(BLKJIT_TEMPS_0(source->size), dest_reg);
	} else {
		assert(false);
	}
	
	if(dest->is_alloc_stack()) {
		Encoder().mov(BLKJIT_TEMPS_1(source->size), GetCompiler().stack_from_operand(dest));
	}

	insn++;
	return true;
}

bool LowerUMul::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *source = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	// can only do multiply with mul instruction (RDX:RAX = RAX * SOURCE)
	// so we need to be very careful about RAX
	
	// 1. is RAX the destination?
	// 2. is RAX the source?
	// 3. is RAX the source AND destination?
	// 4. is RAX neither?
	
	// if RAX is not the destination, then back it up to RDI
	bool rax_is_source = source->is_alloc_reg() && GetCompiler().register_from_operand(source) == REGS_RAX(source->size);
	bool rax_is_dest = dest->is_alloc_reg() && GetCompiler().register_from_operand(dest) == REGS_RAX(dest->size);
	if(!rax_is_dest) {
		Encoder().mov(REG_RAX, REG_RDI);
	}
	
	auto mul_reg = &REG_RIZ;
	// move one of the operands into RAX
	if(rax_is_source && !rax_is_dest) {
		// source is already in RAX, so mul by destination
		if(dest->is_alloc_reg()) {
			mul_reg = &GetCompiler().register_from_operand(dest);
		} else {
			mul_reg = &BLKJIT_TEMPS_1(dest->size);
			GetCompiler().encode_operand_to_reg(dest, *mul_reg);
		}
	} else if(rax_is_dest && !rax_is_source) {
		// dest is already in RAX, so mul by source
		if(source->is_alloc_reg()) {
			mul_reg = &GetCompiler().register_from_operand(source);
		} else {
			mul_reg = &BLKJIT_TEMPS_1(source->size);
			GetCompiler().encode_operand_to_reg(source, *mul_reg);
		}
	} else if(rax_is_source && rax_is_dest) {
		// doesn't matter, mul by source
		if(source->is_alloc_reg()) {
			mul_reg = &GetCompiler().register_from_operand(source);
		} else {
			mul_reg = &BLKJIT_TEMPS_1(source->size);
			GetCompiler().encode_operand_to_reg(source, *mul_reg);
		}
	} else {
		// move source into RAX, then multiply
		GetCompiler().encode_operand_to_reg(source, REGS_RAX(source->size));
		
		if(dest->is_alloc_reg()){
			mul_reg = &GetCompiler().register_from_operand(dest);
		} else {
			mul_reg = &BLKJIT_TEMPS_1(dest->size);
			GetCompiler().encode_operand_to_reg(dest, *mul_reg);
		}
	}
	
	if(dest->size > 1) {
		Encoder().mul(*mul_reg);
	} else {
		Encoder().mul1(*mul_reg);
	}
	
	// put result into destination register
	if(!rax_is_dest) {
		if(dest->is_alloc_reg()) {
			Encoder().mov(REGS_RAX(dest->size), GetCompiler().register_from_operand(dest));
		} else if(dest->is_alloc_stack()) {
			Encoder().mov(REGS_RAX(dest->size), GetCompiler().stack_from_operand(dest));
		}
		
		// also restore RAX
		Encoder().mov(REG_RDI, REG_RAX);
	} else {
		// otherwise, RAX is the destination and the result is already there - do nothing
	}
	
	insn++;
	return true;
}
