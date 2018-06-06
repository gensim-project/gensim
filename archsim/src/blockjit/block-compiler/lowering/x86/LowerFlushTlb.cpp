/*
 * LowerFlushTlb.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "translate/jit_funs.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerFlushTlb::Lower(const captive::shared::IRInstruction *&insn)
{
	GetLoweringContext().emit_save_reg_state(0, GetStackMap(), GetIsStackFixed());
	GetLoweringContext().load_state_field(0, REG_RDI);

	uint64_t fn_ptr = insn->type == IRInstruction::FLUSH_ITLB ? (uint64_t)tmFlushITlb : (uint64_t)tmFlushDTlb;
	Encoder().mov((uint64_t)fn_ptr, REG_RAX);
	Encoder().call(REG_RAX);

	GetLoweringContext().emit_restore_reg_state(GetIsStackFixed());

	insn++;
	return true;
}



bool LowerFlushTlbEntry::Lower(const captive::shared::IRInstruction *&insn)
{
	GetLoweringContext().emit_save_reg_state(1, GetStackMap(), GetIsStackFixed());

	uint64_t fn_ptr = insn->type == IRInstruction::FLUSH_ITLB ? (uint64_t)tmFlushITlbEntry : (uint64_t)tmFlushDTlbEntry;
	GetLoweringContext().load_state_field(0, REG_RDI);
	GetLoweringContext().encode_operand_function_argument(&insn->operands[0], REG_ESI, GetStackMap());

	Encoder().mov((uint64_t)fn_ptr, REG_RAX);
	Encoder().call(REG_RAX);

	GetLoweringContext().emit_restore_reg_state(GetIsStackFixed());

	insn++;
	return true;
}


