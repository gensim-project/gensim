/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/x86/X86Decoder.h"
#include <algorithm>

extern "C" {
#include <xed/xed-interface.h>
}

using namespace archsim;
using namespace archsim::arch::x86;

DeclareLogContext(LogX86Decode, "X86Decode")

#define SPECIAL_RIP_INDEX 0xff
#define SPECIAL_RIZ_INDEX 0xfe

static int get_register_index(xed_reg_enum_t reg)
{
	switch(xed_reg_class(reg)) {
		case XED_REG_CLASS_GPR:
			return xed_get_largest_enclosing_register(reg) - XED_REG_RAX;
		case XED_REG_CLASS_IP:
			return SPECIAL_RIP_INDEX;
		case XED_REG_CLASS_XMM:
			return reg - XED_REG_XMM0;
		case XED_REG_CLASS_MMX:
			return reg - XED_REG_MMX0;

		default:
			UNIMPLEMENTED;
	}
}

static bool is_h_reg(xed_reg_enum_t reg)
{
	return reg >= XED_REG_AH && reg <= XED_REG_BH;
}

uint8_t GetCondition(xed_iclass_enum_t iclass)
{
	switch(iclass) {
		case XED_ICLASS_JB:
		case XED_ICLASS_CMOVB:
		case XED_ICLASS_SETB:
			return 0;
		case XED_ICLASS_JBE:
		case XED_ICLASS_CMOVBE:
		case XED_ICLASS_SETBE:
			return 1;
		case XED_ICLASS_JL:
		case XED_ICLASS_CMOVL:
		case XED_ICLASS_SETL:
			return 2;
		case XED_ICLASS_JLE:
		case XED_ICLASS_CMOVLE:
		case XED_ICLASS_SETLE:
			return 3;
		case XED_ICLASS_JNB:
		case XED_ICLASS_CMOVNB:
		case XED_ICLASS_SETNB:
			return 4;
		case XED_ICLASS_JNBE:
		case XED_ICLASS_CMOVNBE:
		case XED_ICLASS_SETNBE:
			return 5;
		case XED_ICLASS_JNL:
		case XED_ICLASS_CMOVNL:
		case XED_ICLASS_SETNL:
			return 6;
		case XED_ICLASS_JNLE:
		case XED_ICLASS_CMOVNLE:
		case XED_ICLASS_SETNLE:
			return 7;
		case XED_ICLASS_JNO:
		case XED_ICLASS_CMOVNO:
		case XED_ICLASS_SETNO:
			return 8;
		case XED_ICLASS_JNP:
		case XED_ICLASS_CMOVNP:
		case XED_ICLASS_SETNP:
			return 9;
		case XED_ICLASS_JNS:
		case XED_ICLASS_CMOVNS:
		case XED_ICLASS_SETNS:
			return 10;
		case XED_ICLASS_JNZ:
		case XED_ICLASS_CMOVNZ:
		case XED_ICLASS_SETNZ:
			return 11;
		case XED_ICLASS_JO:
		case XED_ICLASS_CMOVO:
		case XED_ICLASS_SETO:
			return 12;
		case XED_ICLASS_JP:
		case XED_ICLASS_CMOVP:
		case XED_ICLASS_SETP:
			return 13;
		case XED_ICLASS_JS:
		case XED_ICLASS_CMOVS:
		case XED_ICLASS_SETS:
			return 14;
		case XED_ICLASS_JZ:
		case XED_ICLASS_CMOVZ:
		case XED_ICLASS_SETZ:
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
			case 64:
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
			output_reg.h_reg = is_h_reg(reg);
			output_reg.index = get_register_index(reg);
			output_reg.width = xed_get_register_width_bits64(reg);
			output_reg.offset = 0;
			break;

		case XED_REG_CLASS_XMM:
			output_reg.index = get_register_index(reg);
			output_reg.width = xed_get_register_width_bits64(reg);
			output_reg.regclass = 1;
			break;

		case XED_REG_CLASS_MMX:
			output_reg.index = get_register_index(reg);
			output_reg.width = xed_get_register_width_bits64(reg);
			output_reg.regclass = 2;
			break;

		case XED_REG_CLASS_IP:
			output_reg.h_reg = false;
			output_reg.index = get_register_index(reg);
			output_reg.width = 8;
			output_reg.offset = xed_decoded_inst_get_length(xedd);
			break;

		case XED_REG_CLASS_INVALID:
		case XED_REG_CLASS_PSEUDO:
		case XED_REG_CLASS_FLAGS:
		case XED_REG_CLASS_PSEUDOX87:
		case XED_REG_CLASS_X87:
		case XED_REG_CLASS_MXCSR:
			// 'zero' register
			output_reg.h_reg = false;
			output_reg.index = SPECIAL_RIZ_INDEX;
			output_reg.width = 8;
			output_reg.offset = 0;
			break;


		default:
			LC_ERROR(LogX86Decode) << "Unknown register type " << xed_reg_enum_t2str(reg) << " (" << xed_reg_class_enum_t2str(xed_reg_class(reg)) << ")";
			return false;
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

		case XED_OPERAND_BASE0:
		case XED_OPERAND_BASE1:
			// do nothing
			break;

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

bool X86Decoder::DecodeOperands(void* inst_)
{
	xed_decoded_inst_t *inst = (xed_decoded_inst_t*)inst_;
	bool success = true;
	ResetOperand(op0);
	ResetOperand(op1);
	ResetOperand(op2);

	auto noperands = xed_decoded_inst_noperands(inst);

	if(noperands > 0) {
		success &= DecodeOperand(inst, 0, op0);
	}

	if(noperands > 1) {
		success &= DecodeOperand(inst, 1, op1);
	}

	if(noperands > 2) {
		success &= DecodeOperand(inst, 2, op2);
	}

	return success;
}

bool X86Decoder::DecodeClass(void* inst_)
{
	xed_decoded_inst_t *inst = (xed_decoded_inst_t*)inst_;
	auto iclass = xed_decoded_inst_get_iclass(inst);

	condition = GetCondition(iclass);

	switch(iclass) {
#define MAP(xed, model) case xed: Instr_Code = model; return true;
			MAP(XED_ICLASS_ADC, INST_x86_adc);
			MAP(XED_ICLASS_ADD, INST_x86_add);
			MAP(XED_ICLASS_ADD_LOCK, INST_x86_add);
			MAP(XED_ICLASS_ADDSD, INST_x86_addsd);
			MAP(XED_ICLASS_ADDSS, INST_x86_addss);
			MAP(XED_ICLASS_AND, INST_x86_and);
			MAP(XED_ICLASS_AND_LOCK, INST_x86_and);
			MAP(XED_ICLASS_ANDPD, INST_x86_andpd);
			MAP(XED_ICLASS_ANDNPD, INST_x86_andnpd);
			MAP(XED_ICLASS_ANDNPS, INST_x86_andnpd); // todo: is this really the same as andnpd
			MAP(XED_ICLASS_ANDPS, INST_x86_andpd); // todo; is this really the same as andpd?
			MAP(XED_ICLASS_BSF, INST_x86_bsf);
			MAP(XED_ICLASS_BSR, INST_x86_bsr);
			MAP(XED_ICLASS_BSWAP, INST_x86_bswap);
			MAP(XED_ICLASS_BT, INST_x86_bt);
			MAP(XED_ICLASS_BTR, INST_x86_btr);
			MAP(XED_ICLASS_BTR_LOCK, INST_x86_btr);
			MAP(XED_ICLASS_BTS, INST_x86_bts);
			MAP(XED_ICLASS_BTS_LOCK, INST_x86_bts);
			MAP(XED_ICLASS_CALL_NEAR, INST_x86_call);
			MAP(XED_ICLASS_CBW, INST_x86_cbw);
			MAP(XED_ICLASS_CDQ, INST_x86_cdq);
			MAP(XED_ICLASS_CDQE, INST_x86_cdqe);
			MAP(XED_ICLASS_CLD, INST_x86_cld);
			MAP(XED_ICLASS_CLFLUSH, INST_x86_clflush);

			MAP(XED_ICLASS_CMOVNB, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVB, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVNBE, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVBE, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVNZ, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVZ, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVNS, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVS, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVNLE, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVLE, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVL, INST_x86_cmov);
			MAP(XED_ICLASS_CMOVNL, INST_x86_cmov);

			MAP(XED_ICLASS_CMP, INST_x86_cmp);
			MAP(XED_ICLASS_CMPSD_XMM, INST_x86_cmpsd_xmm); // thanks again
			MAP(XED_ICLASS_CMPSS, INST_x86_cmpss); // thanks again
			MAP(XED_ICLASS_CMPXCHG, INST_x86_cmpxchg);
			MAP(XED_ICLASS_CMPXCHG_LOCK, INST_x86_cmpxchg);
			MAP(XED_ICLASS_CPUID, INST_x86_cpuid);
			MAP(XED_ICLASS_CQO, INST_x86_cqo);
			MAP(XED_ICLASS_CVTSD2SS, INST_x86_cvtsd2ss);
			MAP(XED_ICLASS_CVTTSD2SI, INST_x86_cvttsd2si);
			MAP(XED_ICLASS_CVTTSS2SI, INST_x86_cvttss2si);
			MAP(XED_ICLASS_CVTSI2SS, INST_x86_cvtsi2ss);
			MAP(XED_ICLASS_CVTSI2SD, INST_x86_cvtsi2sd);
			MAP(XED_ICLASS_CVTSS2SD, INST_x86_cvtss2sd);
			MAP(XED_ICLASS_CWD, INST_x86_cwd);
			MAP(XED_ICLASS_CWDE, INST_x86_cwde);
			MAP(XED_ICLASS_DEC, INST_x86_dec);
			MAP(XED_ICLASS_DEC_LOCK, INST_x86_dec);
			MAP(XED_ICLASS_DIV, INST_x86_div);
			MAP(XED_ICLASS_DIVSD, INST_x86_divsd);
			MAP(XED_ICLASS_DIVSS, INST_x86_divss);
//			MAP(XED_ICLASS_EMMS, INST_x86_nop); // TODO
			MAP(XED_ICLASS_FLD, INST_x86_fld); // TODO
			MAP(XED_ICLASS_FNSTCW, INST_x86_fnstcw); // TODO
			MAP(XED_ICLASS_FST, INST_x86_fst); // TODO
			MAP(XED_ICLASS_FSTP, INST_x86_fstp); // TODO
//			MAP(XED_ICLASS_FXSAVE, INST_x86_nop); // TODO
//			MAP(XED_ICLASS_FXRSTOR, INST_x86_nop); // TODO
			MAP(XED_ICLASS_IDIV, INST_x86_idiv);

		// need to be careful with imul - XED encodes it as a single opcode
		// but we handle it as 3 different instructions
		case XED_ICLASS_IMUL:

			switch(xed_decoded_inst_get_iform_enum(inst)) {
				case XED_IFORM_IMUL_GPR8:
				case XED_IFORM_IMUL_GPRv:
				case XED_IFORM_IMUL_MEMb:
				case XED_IFORM_IMUL_MEMv:
					Instr_Code = INST_x86_imul1;
					return true;
				case XED_IFORM_IMUL_GPRv_GPRv:
				case XED_IFORM_IMUL_GPRv_MEMv:
					Instr_Code = INST_x86_imul2;
					return true;
				case XED_IFORM_IMUL_GPRv_GPRv_IMMb:
				case XED_IFORM_IMUL_GPRv_GPRv_IMMz:
				case XED_IFORM_IMUL_GPRv_MEMv_IMMb:
				case XED_IFORM_IMUL_GPRv_MEMv_IMMz:
					Instr_Code = INST_x86_imul3;
					return true;
				default:
					return false;
			}
			break;

			MAP(XED_ICLASS_INC, INST_x86_inc);
			MAP(XED_ICLASS_INC_LOCK, INST_x86_inc); // TODO: lock

			MAP(XED_ICLASS_JZ, INST_x86_jcond);
			MAP(XED_ICLASS_JNZ, INST_x86_jcond);
			MAP(XED_ICLASS_JBE, INST_x86_jcond);
			MAP(XED_ICLASS_JNBE, INST_x86_jcond);
			MAP(XED_ICLASS_JB, INST_x86_jcond);
			MAP(XED_ICLASS_JNB, INST_x86_jcond);
			MAP(XED_ICLASS_JLE, INST_x86_jcond);
			MAP(XED_ICLASS_JNLE, INST_x86_jcond);
			MAP(XED_ICLASS_JNL, INST_x86_jcond);
			MAP(XED_ICLASS_JL, INST_x86_jcond);
			MAP(XED_ICLASS_JS, INST_x86_jcond);
			MAP(XED_ICLASS_JNS, INST_x86_jcond);
			MAP(XED_ICLASS_JO, INST_x86_jcond);
			MAP(XED_ICLASS_JNO, INST_x86_jcond);
			MAP(XED_ICLASS_JP, INST_x86_jcond);
			MAP(XED_ICLASS_JNP, INST_x86_jcond);

			MAP(XED_ICLASS_JMP, INST_x86_jmp);
			MAP(XED_ICLASS_LEA, INST_x86_lea);
			MAP(XED_ICLASS_LDMXCSR, INST_x86_nop);
			MAP(XED_ICLASS_LEAVE, INST_x86_leave);
			MAP(XED_ICLASS_MAXSD, INST_x86_maxsd);
			MAP(XED_ICLASS_MAXSS, INST_x86_maxss);
			MAP(XED_ICLASS_MFENCE, INST_x86_nop); // TODO: think about fences and other architectures
			MAP(XED_ICLASS_MINSS, INST_x86_minss);
			MAP(XED_ICLASS_MOV, INST_x86_mov);
			MAP(XED_ICLASS_MOVD, INST_x86_movd);
			MAP(XED_ICLASS_MOVDQU, INST_x86_movdqu);
			MAP(XED_ICLASS_MOVDQA, INST_x86_movdqu);
			MAP(XED_ICLASS_MOVNTDQ, INST_x86_movdqu);
			MAP(XED_ICLASS_MOVQ, INST_x86_movq);
			MAP(XED_ICLASS_MOVLPD, INST_x86_movlpd);
			MAP(XED_ICLASS_MOVMSKPD, INST_x86_movmskpd);
			MAP(XED_ICLASS_MOVHPD, INST_x86_movhpd);
			MAP(XED_ICLASS_MOVHPS, INST_x86_movhps);
			MAP(XED_ICLASS_MOVSB, INST_x86_movsb);
			MAP(XED_ICLASS_MOVSD, INST_x86_movsd);
			MAP(XED_ICLASS_MOVSD_XMM, INST_x86_movsd_xmm); // very helpful that two totally different instructions have the same name
			MAP(XED_ICLASS_MOVSQ, INST_x86_movsq);
			MAP(XED_ICLASS_MOVSS, INST_x86_movss);
			MAP(XED_ICLASS_MOVSXD, INST_x86_movsxd);
			MAP(XED_ICLASS_MOVSX, INST_x86_movsx);
			MAP(XED_ICLASS_MOVAPS, INST_x86_movups);
			MAP(XED_ICLASS_MOVUPS, INST_x86_movups);
			MAP(XED_ICLASS_MOVAPD, INST_x86_movups);
			MAP(XED_ICLASS_MOVUPD, INST_x86_movups);
			MAP(XED_ICLASS_MOVNTI, INST_x86_mov);
			MAP(XED_ICLASS_MOVZX, INST_x86_movzx);
			MAP(XED_ICLASS_MUL, INST_x86_mul);
			MAP(XED_ICLASS_MULSD, INST_x86_mulsd);
			MAP(XED_ICLASS_MULSS, INST_x86_mulss);
			MAP(XED_ICLASS_NEG, INST_x86_neg);
			MAP(XED_ICLASS_NOP, INST_x86_nop);
			MAP(XED_ICLASS_NOT, INST_x86_not);
			MAP(XED_ICLASS_OR, INST_x86_or);
			MAP(XED_ICLASS_OR_LOCK, INST_x86_or);
			MAP(XED_ICLASS_ORPD, INST_x86_por); // Is this really the same?
			MAP(XED_ICLASS_ORPS, INST_x86_por); // Is this really the same?
			MAP(XED_ICLASS_PACKUSWB, INST_x86_packuswb);
			MAP(XED_ICLASS_PADDB, INST_x86_paddb);
			MAP(XED_ICLASS_PADDD, INST_x86_paddd);
			MAP(XED_ICLASS_PADDQ, INST_x86_paddq);
			MAP(XED_ICLASS_PAND, INST_x86_pand);
			MAP(XED_ICLASS_PCMPEQB, INST_x86_pcmpeqb);
			MAP(XED_ICLASS_PCMPEQD, INST_x86_pcmpeqd);
			MAP(XED_ICLASS_PCMPGTB, INST_x86_pcmpgtb);
			MAP(XED_ICLASS_PCMPGTW, INST_x86_pcmpgtw);
			MAP(XED_ICLASS_PCMPGTD, INST_x86_pcmpgtd);
			MAP(XED_ICLASS_PMOVMSKB, INST_x86_pmovmskb);
			MAP(XED_ICLASS_PMAXUB, INST_x86_pmaxub);
			MAP(XED_ICLASS_PMINUB, INST_x86_pminub);
			MAP(XED_ICLASS_POP, INST_x86_pop);

			// TODO: fix variants of popf
			MAP(XED_ICLASS_POPF, INST_x86_popf);
			MAP(XED_ICLASS_POPFD, INST_x86_popf);
			MAP(XED_ICLASS_POPFQ, INST_x86_popf);

			MAP(XED_ICLASS_POR, INST_x86_por);
			MAP(XED_ICLASS_PSUBB, INST_x86_psubb);
			MAP(XED_ICLASS_PUNPCKLBW, INST_x86_punpcklbw);
			MAP(XED_ICLASS_PUNPCKLWD, INST_x86_punpcklwd);
			MAP(XED_ICLASS_PUNPCKLDQ, INST_x86_punpckldq);
			MAP(XED_ICLASS_PUNPCKHWD, INST_x86_punpckhwd);
			MAP(XED_ICLASS_PUNPCKHDQ, INST_x86_punpckhdq);
			MAP(XED_ICLASS_PUNPCKHQDQ, INST_x86_punpckhqdq);
			MAP(XED_ICLASS_PUNPCKLQDQ, INST_x86_punpcklqdq);
			MAP(XED_ICLASS_PUSH, INST_x86_push);
			MAP(XED_ICLASS_PUSHFQ, INST_x86_pushfq);
			MAP(XED_ICLASS_PSHUFD, INST_x86_pshufd);
			MAP(XED_ICLASS_PSLLD, INST_x86_pslld);
			MAP(XED_ICLASS_PSLLQ, INST_x86_psllq);
			MAP(XED_ICLASS_PSLLDQ, INST_x86_pslldq);
			MAP(XED_ICLASS_PREFETCHT0, INST_x86_nop);
			MAP(XED_ICLASS_PREFETCHT1, INST_x86_nop);
			MAP(XED_ICLASS_PREFETCHT2, INST_x86_nop);
			MAP(XED_ICLASS_PSRAD, INST_x86_psrad);
			MAP(XED_ICLASS_PSRLDQ, INST_x86_psrldq);
			MAP(XED_ICLASS_PSRLQ, INST_x86_psrlq);
			MAP(XED_ICLASS_PXOR, INST_x86_pxor);
			MAP(XED_ICLASS_RDTSC, INST_x86_rdtsc);
			MAP(XED_ICLASS_REPE_CMPSB, INST_x86_repe_cmpsb);
			MAP(XED_ICLASS_REP_MOVSB, INST_x86_rep_movsb);
			MAP(XED_ICLASS_REP_MOVSD, INST_x86_rep_movsd);
			MAP(XED_ICLASS_REP_MOVSQ, INST_x86_rep_movsq);
			MAP(XED_ICLASS_REP_STOSB, INST_x86_rep_stosb);
			MAP(XED_ICLASS_REP_STOSD, INST_x86_rep_stosd);
			MAP(XED_ICLASS_REP_STOSQ, INST_x86_rep_stosq);
			MAP(XED_ICLASS_REPNE_SCASB, INST_x86_repne_scasb);
			MAP(XED_ICLASS_RET_NEAR, INST_x86_ret);
			MAP(XED_ICLASS_ROL, INST_x86_rol);
			MAP(XED_ICLASS_ROR, INST_x86_ror);
			MAP(XED_ICLASS_SAR, INST_x86_sar);
			MAP(XED_ICLASS_SBB, INST_x86_sbb);

			MAP(XED_ICLASS_SETBE, INST_x86_setcc);
			MAP(XED_ICLASS_SETNBE, INST_x86_setcc);
			MAP(XED_ICLASS_SETB, INST_x86_setcc);
			MAP(XED_ICLASS_SETNB, INST_x86_setcc);
			MAP(XED_ICLASS_SETZ, INST_x86_setcc);
			MAP(XED_ICLASS_SETNZ, INST_x86_setcc);
			MAP(XED_ICLASS_SETLE, INST_x86_setcc);
			MAP(XED_ICLASS_SETNLE, INST_x86_setcc);
			MAP(XED_ICLASS_SETL, INST_x86_setcc);
			MAP(XED_ICLASS_SETNL, INST_x86_setcc);
			MAP(XED_ICLASS_SETP, INST_x86_setcc);
			MAP(XED_ICLASS_SETNP, INST_x86_setcc);
			MAP(XED_ICLASS_SETS, INST_x86_setcc);
			MAP(XED_ICLASS_SETNS, INST_x86_setcc);

			MAP(XED_ICLASS_SFENCE, INST_x86_nop);
			MAP(XED_ICLASS_SHL, INST_x86_shl);
			MAP(XED_ICLASS_SHLD, INST_x86_shld);
			MAP(XED_ICLASS_SHR, INST_x86_shr);
			MAP(XED_ICLASS_SHRD, INST_x86_shrd);
			MAP(XED_ICLASS_SQRTSD, INST_x86_sqrtsd);
			MAP(XED_ICLASS_STMXCSR, INST_x86_nop); // TODO
			MAP(XED_ICLASS_SUB, INST_x86_sub);
			MAP(XED_ICLASS_SUB_LOCK, INST_x86_sub);
			MAP(XED_ICLASS_SUBSD, INST_x86_subsd);
			MAP(XED_ICLASS_SUBSS, INST_x86_subss);
			MAP(XED_ICLASS_SYSCALL, INST_x86_syscall);
			MAP(XED_ICLASS_TEST, INST_x86_test);
			MAP(XED_ICLASS_TZCNT, INST_x86_tzcnt);
			MAP(XED_ICLASS_UCOMISD, INST_x86_ucomisd);
			MAP(XED_ICLASS_UCOMISS, INST_x86_ucomiss);
			MAP(XED_ICLASS_UNPCKLPD, INST_x86_unpcklpd);
			MAP(XED_ICLASS_XADD, INST_x86_xadd);
			MAP(XED_ICLASS_XADD_LOCK, INST_x86_xadd);
			MAP(XED_ICLASS_XCHG, INST_x86_xchg);
			MAP(XED_ICLASS_XGETBV, INST_x86_xgetbv);
			MAP(XED_ICLASS_XOR, INST_x86_xor);
			MAP(XED_ICLASS_XOR_LOCK, INST_x86_xor);
			MAP(XED_ICLASS_XORPD, INST_x86_xorpd);

		default:
			return false;
#undef MAP
	}
}

bool X86Decoder::DecodeFlow(void* inst_)
{
	xed_decoded_inst_t *inst = (xed_decoded_inst_t*)inst_;
	auto category = xed_decoded_inst_get_category(inst);
	auto iclass = xed_decoded_inst_get_iclass(inst);
	ClearEndOfBlock();
	ClearIsPredicated();
	is_predicated = 0;

	if(iclass >= XED_ICLASS_REPE_CMPSB && iclass <= XED_ICLASS_REP_STOSW) {
		SetEndOfBlock();
	}

	switch(category) {
		case XED_CATEGORY_COND_BR:
		case XED_CATEGORY_UNCOND_BR:
		case XED_CATEGORY_CALL:
		case XED_CATEGORY_RET:
			SetEndOfBlock();
			break;
		default:
			break;
	}

	if(category == XED_CATEGORY_COND_BR) {
		is_predicated = 1;
		SetIsPredicated();
		condition = GetCondition(iclass);
	}

	return true;
}

bool X86Decoder::DecodeLock(void* inst)
{
	xed_decoded_inst_t *xedd = (xed_decoded_inst_t*)inst;

	// No really easy way to tell if a instruction is locked so just look at
	// all of the lockable iclasses
	switch(xed_decoded_inst_get_iclass(xedd)) {
		case XED_ICLASS_ADC_LOCK:
		case XED_ICLASS_ADD_LOCK:
		case XED_ICLASS_AND_LOCK:
		case XED_ICLASS_BTC_LOCK:
		case XED_ICLASS_BTR_LOCK:
		case XED_ICLASS_BTS_LOCK:
		case XED_ICLASS_CMPXCHG_LOCK:
		case XED_ICLASS_CMPXCHG8B_LOCK:
		case XED_ICLASS_CMPXCHG16B_LOCK:
		case XED_ICLASS_DEC_LOCK:
		case XED_ICLASS_INC_LOCK:
		case XED_ICLASS_NEG_LOCK:
		case XED_ICLASS_NOT_LOCK:
		case XED_ICLASS_OR_LOCK:
		case XED_ICLASS_SBB_LOCK:
		case XED_ICLASS_SUB_LOCK:
		case XED_ICLASS_XOR_LOCK:
		case XED_ICLASS_XADD_LOCK:
		case XED_ICLASS_XCHG:
			lock = true;
			break;
		default:
			lock = false;
			break;
	}

	return true;
}

int X86Decoder::DecodeInstr(Address addr, int mode, MemoryInterface& interface)
{
	// read 15 bytes
	uint8_t data[15];
	if(interface.Read(addr, data, 15) != MemoryResult::OK) {
		return 1;
	}

	// pass data to XED
	xed_state_t dstate;
	xed_decoded_inst_t xedd;
	xed_error_enum_t xed_error;

	static bool tables_inited = false;
	if(!tables_inited) {
		xed_tables_init();
		tables_inited = true;
	}
	xed_state_zero(&dstate);
	dstate.mmode = XED_MACHINE_MODE_LONG_64;

	xed_decoded_inst_zero_set_mode(&xedd, &dstate);
	xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_HASWELL);

	xed_error = xed_decode(&xedd, XED_REINTERPRET_CAST(const xed_uint8_t*, data), 15);

	Instr_Length = xed_decoded_inst_get_length(&xedd);

	uint8_t irdata[4] = {0};
	for(int i = 0; i < std::min((uint8_t)4, Instr_Length); ++i) {
		irdata[i] = data[i];
	}
	ir = *(uint32_t*)irdata;

	bool success = true;

	if(!DecodeOperands((void*)&xedd)) {
		printf("failed to decode operands\n");
		success = false;
	}
	if(!DecodeFlow((void*)&xedd)) {
		printf("Failed to decode flow\n");
		success = false;
	}
	if(!DecodeClass((void*)&xedd)) {
		printf("Failed to decode class\n");
		success = false;
	}
	if(!DecodeLock((void*)&xedd)) {
		printf("Failed to decode lock\n");
		success = false;
	}

	if(!success || (archsim::options::Verbose && archsim::options::Debug)) {
		xed_decoded_inst_dump(&xedd, dump_buffer, sizeof(dump_buffer));
		printf("%p decode error\n", this);
		printf("%p (%u) %s\n", addr.Get(), Instr_Length, dump_buffer);

	}

	ASSERT(Instr_Length == xed_decoded_inst_get_length(&xedd));

	return !success;
}
