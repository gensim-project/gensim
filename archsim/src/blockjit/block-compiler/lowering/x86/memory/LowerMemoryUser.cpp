/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/lowering/Finalisation.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"
#include "translate/jit_funs.h"

#include "define.h"

#include "gensim/gensim_processor_state.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerReadMemUser::TxlnStart(captive::arch::jit::BlockCompiler *compiler, captive::arch::jit::TranslationContext *ctx)
{
	return true;
}

bool LowerWriteMemUser::TxlnStart(captive::arch::jit::BlockCompiler *compiler, captive::arch::jit::TranslationContext *ctx)
{
	return true;
}

bool LowerReadMemUser::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand *offset = &insn->operands[0];
	const IROperand *disp = &insn->operands[1];
	const IROperand *dest = &insn->operands[2];

	assert(disp->is_constant());
	// We can have the situation where dest is not allocated because the intervening register write has been eliminated
	const auto &dest_reg = dest->is_allocated() && !dest->is_alloc_stack() ? GetCompiler().register_from_operand(dest) : GetCompiler().get_temp(0, dest->size);

	assert(!dest->is_alloc_stack());

	if(disp->value != 0) {
		if(offset->is_alloc_reg()) {
			Encoder().mov(X86Memory::get(GetCompiler().register_from_operand(offset, 8), disp->value).withSegment(1), dest_reg);
			insn++;
			return true;
		} else if(offset->is_alloc_stack()) {
			Encoder().mov(GetCompiler().stack_from_operand(offset), BLKJIT_ARG1(4));
			Encoder().mov(X86Memory::get(BLKJIT_ARG1(8), disp->value).withSegment(1), dest_reg);
		} else {
			assert(false);
		}

	} else {
		if(offset->is_alloc_reg()) {
			Encoder().mov(X86Memory::get(GetCompiler().register_from_operand(offset, 8)).withSegment(1), dest_reg);
		} else if(offset->is_alloc_stack()) {
			Encoder().mov(GetCompiler().stack_from_operand(offset), BLKJIT_ARG1(4));
			Encoder().mov(X86Memory::get(BLKJIT_ARG1(8)).withSegment(1), dest_reg);
		} else {
			return false;
		}
	}

	Encoder().byteswap(dest_reg);

	insn++;
	return true;
}

bool LowerWriteMemUser::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand *value = &insn->operands[0];
	const IROperand *disp = &insn->operands[1];
	const IROperand *offset = &insn->operands[2];

	assert(disp->is_constant());

	Encoder().byteswap(GetCompiler().register_from_operand(value));
	
	if(disp->value != 0) {
		if(offset->is_alloc_reg()) {
			Encoder().lea(X86Memory::get(GetCompiler().register_from_operand(offset, 4), disp->value), BLKJIT_ARG1(4));
		} else if(offset->is_alloc_stack()) {
			Encoder().mov(GetCompiler().stack_from_operand(offset), BLKJIT_ARG1(4));
			Encoder().add(disp->value, BLKJIT_ARG1(4));
		} else {
			assert(false);
		}
	} else {
		if(offset->is_alloc_reg())
			Encoder().mov(GetCompiler().register_from_operand(offset, 4), BLKJIT_ARG1(4));
		else
			Encoder().mov(GetCompiler().stack_from_operand(offset), BLKJIT_ARG1(4));
	}

	Encoder().mov(0x40000000, BLKJIT_ARG0(8));
	if(value->is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(value), BLKJIT_ARG2(value->size));
		Encoder().mov(BLKJIT_ARG2(value->size), X86Memory::get(BLKJIT_ARG1(8), BLKJIT_ARG0(8), 2));
	} else if(value->is_alloc_reg()) {
		Encoder().mov(GetCompiler().register_from_operand(value), X86Memory::get(BLKJIT_ARG1(8), BLKJIT_ARG0(8), 2));
	} else if(value->is_constant()) {
		Encoder().mov(value->value, BLKJIT_ARG2(value->size));
		Encoder().mov(BLKJIT_ARG2(value->size), X86Memory::get(BLKJIT_ARG1(8), BLKJIT_ARG0(8), 2));
	}

	insn++;
	return true;
}