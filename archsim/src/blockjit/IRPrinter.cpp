/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "blockjit/IRPrinter.h"
#include <iomanip>

using namespace archsim::blockjit;
using namespace captive::arch::jit;
using namespace captive::shared;

static void dump_insn(const IRInstruction *insn, std::ostream &str)
{
	assert(insn->type < captive::shared::num_descriptors);
	const struct insn_descriptor *descr = &insn_descriptors[insn->type];

	str << " " << std::left << std::setw(12) << std::setfill(' ') << descr->mnemonic;

	for (int op_idx = 0; op_idx < 6; op_idx++) {
		const IROperand *oper = &insn->operands[op_idx];

		if (descr->format[op_idx] != 'X') {
			if (descr->format[op_idx] == 'M' && !oper->is_valid()) continue;

			if (op_idx > 0) str << ", ";

			if (oper->is_vreg()) {
				char alloc_char = oper->alloc_mode == IROperand::NOT_ALLOCATED ? 'N' : (oper->alloc_mode == IROperand::ALLOCATED_REG ? 'R' : (oper->alloc_mode == IROperand::ALLOCATED_STACK ? 'S' : '?'));
				str << "i" << (uint32_t)oper->size << " r" << std::dec << oper->value << "(" << alloc_char << oper->alloc_data << ")";
			} else if (oper->is_constant()) {
				str << "i" << (uint32_t)oper->size << " $0x" << std::hex << oper->value;
			} else if (oper->is_pc()) {
				str << "i4 pc (" << std::hex << oper->value << ")";
			} else if (oper->is_block()) {
				str << "b" << std::dec << oper->value;
			} else if (oper->is_func()) {
				str << "&" << std::hex << oper->value;
			} else {
				str << "<invalid>";
			}
		}
	}
}


void IRPrinter::DumpIR(std::ostream& ostr, const captive::arch::jit::TranslationContext& ctx) {
	IRBlockId current_block_id = INVALID_BLOCK_ID;

	for (uint32_t ir_idx = 0; ir_idx < ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);

		if (current_block_id != insn->ir_block) {
			current_block_id = insn->ir_block;
			ostr << "block " << std::hex << current_block_id << ":\n";
		}

		dump_insn(insn, ostr);

		ostr << std::endl;
	}
}