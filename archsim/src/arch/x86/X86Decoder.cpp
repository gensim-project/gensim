/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/x86/X86Decoder.h"

extern "C" {
#include <xed/xed-interface.h>
}

using namespace archsim;
using namespace archsim::arch::x86;

#define SPECIAL_RIP_INDEX 0xff
#define SPECIAL_RIZ_INDEX 0xfe

static int get_register_index(xed_reg_enum_t reg)
{
	switch(xed_reg_class(reg)) {
		case XED_REG_CLASS_GPR:
			return xed_get_largest_enclosing_register(reg) - XED_REG_RAX;
		case XED_REG_CLASS_IP:
			return SPECIAL_RIP_INDEX;
		default:
			UNIMPLEMENTED;
	}
}

uint8_t GetCondition(xed_iclass_enum_t iclass)
{
	switch(iclass) {
		case XED_ICLASS_JB:
		case XED_ICLASS_CMOVB:
			return 0;
		case XED_ICLASS_JBE:
		case XED_ICLASS_CMOVBE:
			return 1;
		case XED_ICLASS_JL:
		case XED_ICLASS_CMOVL:
			return 2;
		case XED_ICLASS_JLE:
		case XED_ICLASS_CMOVLE:
			return 3;
		case XED_ICLASS_JNB:
		case XED_ICLASS_CMOVNB:
			return 4;
		case XED_ICLASS_JNBE:
		case XED_ICLASS_CMOVNBE:
			return 5;
		case XED_ICLASS_JNL:
		case XED_ICLASS_CMOVNL:
			return 6;
		case XED_ICLASS_JNLE:
		case XED_ICLASS_CMOVNLE:
			return 7;
		case XED_ICLASS_JNO:
		case XED_ICLASS_CMOVNO:
			return 8;
		case XED_ICLASS_JNP:
		case XED_ICLASS_CMOVNP:
			return 9;
		case XED_ICLASS_JNS:
		case XED_ICLASS_CMOVNS:
			return 10;
		case XED_ICLASS_JNZ:
		case XED_ICLASS_CMOVNZ:
			return 11;
		case XED_ICLASS_JO:
		case XED_ICLASS_CMOVO:
			return 12;
		case XED_ICLASS_JP:
		case XED_ICLASS_CMOVP:
			return 13;
		case XED_ICLASS_JS:
		case XED_ICLASS_CMOVS:
			return 14;
		case XED_ICLASS_JZ:
		case XED_ICLASS_CMOVZ:
			return 15;

		// Unconditional
		default:
			return -1;
	}
}

uint64_t UnpackImmediate(uint64_t bits, bool is_signed, uint8_t width_bits)
{
	uint64_t value = bits;
	if(is_signed) {
		// sign extend from width_bits to 64 bits
		switch(width_bits) {
			case 0:
				value = 0;
				break;
			case 8:
				value = (int64_t)(int8_t)value;
				break;
			case 16:
				value = (int64_t)(int16_t)value;
				break;
			case 32:
				value = (int64_t)(int32_t)value;
				break;
			default:
				UNIMPLEMENTED;
		}
	}
	return value;
}

bool DecodeRegister(xed_decoded_inst_t *xedd, xed_reg_enum_t reg, X86Decoder::Register &output_reg)
{
	switch(xed_reg_class(reg)) {
		case XED_REG_CLASS_GPR:
			output_reg.index = get_register_index(reg);
			output_reg.width = xed_get_register_width_bits64(reg);
			output_reg.offset = 0;
			break;

		case XED_REG_CLASS_IP:
			output_reg.index = get_register_index(reg);
			output_reg.width = 8;
			output_reg.offset = xed_decoded_inst_get_length(xedd);
			break;

		case XED_REG_CLASS_INVALID:
		case XED_REG_CLASS_PSEUDO:
		case XED_REG_CLASS_FLAGS:
			// 'zero' register
			output_reg.index = SPECIAL_RIZ_INDEX;
			output_reg.width = 8;
			output_reg.offset = 0;
			break;


		default:
			UNIMPLEMENTED;
			break;
	}

	return true;
}

bool DecodeRegisterOperand(xed_decoded_inst_t *xedd, xed_operand_enum_t op_name, X86Decoder::Operand &output_op)
{
	auto reg = xed_decoded_inst_get_reg(xedd, op_name);
	output_op.is_reg = 1;
	return DecodeRegister(xedd, reg, output_op.reg);
}

bool DecodeMemory(xed_decoded_inst_t *xedd, xed_operand_enum_t op_name, X86Decoder::Operand &output_op)
{
	int mem_idx = 0;
	if(op_name == XED_OPERAND_MEM1) {
		mem_idx = 1;
	}

	output_op.is_mem = true;

	auto seg_reg = xed_decoded_inst_get_seg_reg(xedd, mem_idx);
	if(seg_reg != XED_REG_INVALID) {
		output_op.memory.has_segment = true;
		output_op.memory.segment = seg_reg - XED_REG_SR_FIRST;
	}

	auto base_reg = xed_decoded_inst_get_base_reg(xedd, mem_idx);
	DecodeRegister(xedd, base_reg, output_op.memory.base);

	auto idx_reg = xed_decoded_inst_get_index_reg(xedd, mem_idx);
	DecodeRegister(xedd, idx_reg, output_op.memory.index);
	output_op.memory.scale = xed_decoded_inst_get_scale(xedd, mem_idx);

	uint64_t disp_value = xed_decoded_inst_get_memory_displacement(xedd, mem_idx);
	uint8_t disp_size = xed_decoded_inst_get_memory_displacement_width_bits(xedd, mem_idx);
	output_op.memory.displacement = UnpackImmediate(disp_value, true, disp_size);
	output_op.memory.width = xed_decoded_inst_get_memory_operand_length(xedd, mem_idx) * 8;

	return true;
}


bool DecodeImmediate(xed_decoded_inst_t *xedd, xed_operand_enum_t op_name, X86Decoder::Operand &output_op)
{
	uint64_t value = xed_decoded_inst_get_unsigned_immediate(xedd);
	uint8_t width_bits = xed_decoded_inst_get_immediate_width_bits(xedd);
	bool is_signed = xed_decoded_inst_get_immediate_is_signed(xedd);

	output_op.imm.value = UnpackImmediate(value, is_signed, width_bits);
	output_op.is_imm = true;

	return true;
}

bool DecodeRelBr(xed_decoded_inst_t *xedd, xed_operand_enum_t op_name, X86Decoder::Operand &output_op)
{
	uint64_t value = xed_decoded_inst_get_branch_displacement(xedd);
	uint8_t width_bits = xed_decoded_inst_get_branch_displacement_width_bits(xedd);

	output_op.is_relbr = true;
	output_op.imm.value = UnpackImmediate(value, true, width_bits) + xed_decoded_inst_get_length(xedd);

	return true;
}

bool DecodeOperand(xed_decoded_inst_t *xedd, int op_idx, X86Decoder::Operand &output_op)
{
	auto inst = xed_decoded_inst_inst(xedd);
	auto operand = xed_inst_operand(inst, op_idx);
	auto op_name = xed_operand_name(operand);
	switch(op_name) {
		case XED_OPERAND_REG0:
		case XED_OPERAND_REG1:
		case XED_OPERAND_REG2:
		case XED_OPERAND_REG3:
		case XED_OPERAND_REG4:
		case XED_OPERAND_REG5:
		case XED_OPERAND_REG6:
		case XED_OPERAND_REG7:
		case XED_OPERAND_REG8:
			return DecodeRegisterOperand(xedd, op_name, output_op);

		case XED_OPERAND_AGEN:
		case XED_OPERAND_MEM0:
		case XED_OPERAND_MEM1:
			return DecodeMemory(xedd, op_name, output_op);

		case XED_OPERAND_IMM0:
		case XED_OPERAND_IMM0SIGNED:
		case XED_OPERAND_IMM1:
			return DecodeImmediate(xedd, op_name, output_op);

		case XED_OPERAND_RELBR:
			return DecodeRelBr(xedd, op_name, output_op);

		default:
			UNIMPLEMENTED;
			break;
	}

	return true;
}

char dump_buffer[2048];

static void ResetOperand(X86Decoder::Operand &op)
{
	bzero((void*)&op, sizeof(op));
}

X86Decoder::X86Decoder()
{

}

void X86Decoder::DecodeOperands(void* inst_)
{
	xed_decoded_inst_t *inst = (xed_decoded_inst_t*)inst_;
	ResetOperand(op0);
	ResetOperand(op1);
	ResetOperand(op2);

	auto noperands = xed_decoded_inst_noperands(inst);

	if(noperands > 0) {
		DecodeOperand(inst, 0, op0);
	}

	if(noperands > 1) {
		DecodeOperand(inst, 1, op1);
	}

	if(noperands > 2) {
		DecodeOperand(inst, 2, op2);
	}
}

void X86Decoder::DecodeClass(void* inst_)
{
	xed_decoded_inst_t *inst = (xed_decoded_inst_t*)inst_;
	auto iclass = xed_decoded_inst_get_iclass(inst);

	condition = GetCondition(iclass);

	switch(iclass) {
#define MAP(xed, model) case xed: Instr_Code = model; break;
			MAP(XED_ICLASS_ADD, INST_x86_add);
			MAP(XED_ICLASS_AND, INST_x86_and);
			MAP(XED_ICLASS_CALL_NEAR, INST_x86_call);

			MAP(XED_ICLASS_CMOVNZ, INST_x86_cmov);

			MAP(XED_ICLASS_CMP, INST_x86_cmp);
			MAP(XED_ICLASS_CPUID, INST_x86_cpuid);

			MAP(XED_ICLASS_JZ, INST_x86_jcond);
			MAP(XED_ICLASS_JNZ, INST_x86_jcond);
			MAP(XED_ICLASS_JBE, INST_x86_jcond);
			MAP(XED_ICLASS_JNBE, INST_x86_jcond);
			MAP(XED_ICLASS_JLE, INST_x86_jcond);

			MAP(XED_ICLASS_JMP, INST_x86_jmp);
			MAP(XED_ICLASS_LEA, INST_x86_lea);
			MAP(XED_ICLASS_MOV, INST_x86_mov);
			MAP(XED_ICLASS_MOVSXD, INST_x86_movsxd);
			MAP(XED_ICLASS_MOVZX, INST_x86_movzx);
			MAP(XED_ICLASS_NOP, INST_x86_nop);
			MAP(XED_ICLASS_OR, INST_x86_or);
			MAP(XED_ICLASS_POP, INST_x86_pop);
			MAP(XED_ICLASS_PUSH, INST_x86_push);
			MAP(XED_ICLASS_RET_NEAR, INST_x86_ret);
			MAP(XED_ICLASS_SETZ, INST_x86_setz);
			MAP(XED_ICLASS_SUB, INST_x86_sub);
			MAP(XED_ICLASS_SYSCALL, INST_x86_syscall);
			MAP(XED_ICLASS_TEST, INST_x86_test);
			MAP(XED_ICLASS_XGETBV, INST_x86_xgetbv);
			MAP(XED_ICLASS_XOR, INST_x86_xor);

		default:
			UNIMPLEMENTED;
#undef MAP
	}
}

void X86Decoder::DecodeFlow(void* inst_)
{
	xed_decoded_inst_t *inst = (xed_decoded_inst_t*)inst_;
	auto category = xed_decoded_inst_get_category(inst);

	switch(category) {
		case XED_CATEGORY_COND_BR:
		case XED_CATEGORY_UNCOND_BR:
		case XED_CATEGORY_CALL:
		case XED_CATEGORY_RET:
			SetEndOfBlock();
			break;
		default:
			ClearEndOfBlock();
			break;
	}
}


int X86Decoder::DecodeInstr(Address addr, int mode, MemoryInterface& interface)
{
	// read 15 bytes
	uint8_t data[15];
	interface.Read(addr, data, 15);

	// pass data to XED
	xed_state_t dstate;
	xed_decoded_inst_t xedd;
	xed_error_enum_t xed_error;

	xed_tables_init();
	xed_state_zero(&dstate);
	dstate.mmode = XED_MACHINE_MODE_LONG_64;

	xed_decoded_inst_zero_set_mode(&xedd, &dstate);
	xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_HASWELL);

	xed_error = xed_decode(&xedd, XED_REINTERPRET_CAST(const xed_uint8_t*, data), 15);

	Instr_Length = xed_decoded_inst_get_length(&xedd);

	if(archsim::options::Verbose) {
		xed_decoded_inst_dump(&xedd, dump_buffer, sizeof(dump_buffer));
		printf("%p (%u) %s\n", addr.Get(), Instr_Length, dump_buffer);
	}

	DecodeOperands((void*)&xedd);
	DecodeFlow((void*)&xedd);
	DecodeClass((void*)&xedd);


	return 0;
}
