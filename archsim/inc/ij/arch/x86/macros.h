/*
 * macros.h
 */

#ifndef MACROS_H_
#define MACROS_H_

#define X86_MACROS_INTERNAL

#include <cassert>

#include "registers.h"

namespace x86
{



	using namespace reg;

	inline static void EMIT_BYTE(uint8_t*& code_buffer, uint8_t byte)
	{
		*code_buffer = byte;
		code_buffer++;
	}

#define E(byte) EMIT_BYTE(code_buffer, byte)

	template<uint8_t length> inline static void EMIT_CONSTANT(uint8_t *&code_buffer, uint64_t imm);

	template<> inline void EMIT_CONSTANT<1>(uint8_t *&code_buffer, uint64_t imm)
	{
		E(imm & 0xff);
	}

	template<> inline void EMIT_CONSTANT<2>(uint8_t *&code_buffer, uint64_t imm)
	{
		E(imm & 0xff);
		E((imm & 0xff00) >> 8);
	}

	template<> inline void EMIT_CONSTANT<4>(uint8_t *&code_buffer, uint64_t imm)
	{
		uint32_t *b = (uint32_t*)code_buffer;
		*b = (imm & 0xffffffff);
		b++;
		code_buffer = (uint8_t*)b;
	}

	template<> inline void EMIT_CONSTANT<8>(uint8_t *&code_buffer, uint64_t imm)
	{
		uint64_t *b = (uint64_t*)code_buffer;
		*b = (imm);
		b++;
		code_buffer = (uint8_t*)b;
	}

	inline static void EMIT_REX_BYTE(uint8_t *&code_buffer, bool b, bool x, bool r, bool w)
	{
		uint8_t rex = 0x40;

		if (b) rex |= 0x01;
		if (x) rex |= 0x02;
		if (r) rex |= 0x04;
		if (w) rex |= 0x08;

		E(rex);
	}

	inline static void EMIT_REX(uint8_t *&code_buffer, bool b64, bool hsrc, bool hdst)
	{
		uint8_t rex = 0x40;
		if(b64) rex |= 0x8;
		if(hsrc) rex |= 0x4;
		if(hdst) rex |= 0x1;
		if(rex != 0x40)E(rex);
	}

	inline static void EMIT_MODRM(uint8_t *&code_buffer, uint8_t mod, uint8_t reg, uint8_t rm)
	{
		uint8_t modrm = mod << 6;
		modrm |= reg << 3;
		modrm |= rm;
		E(modrm);
	}

	inline static void EMIT_SIB(uint8_t*& code_buffer, uint8_t scale, uint8_t index, uint8_t base)
	{
		uint8_t sib = 0;
		sib |= scale << 6;
		sib |= index << 3;
		sib |= base;
		E(sib);
	}

	namespace X86OpType
	{
		enum EX86AluOpType {
			o_add = 0,
			o_bor,
			o_adc,
			o_sbb,
			o_band,
			o_sub,
			o_bxor,
			o_cmp,
		};

		enum EX86BitOpType {
			o_rol = 0,
			o_ror,
			o_rcl,
			o_rcr,
			o_shl,
			o_shr,
			o_sal,
			o_sar,
		};

		enum EX86JmpOpType {
			o_jo = 0,
			o_jno = 1,
			o_jb = 2,
			o_jnae = 2,
			o_jc = 2,
			o_jnb = 3,
			o_jae = 3,
			o_jnc = 3,
			o_jz = 4,
			o_je = 4,
			o_jnz = 5,
			o_jne = 5,
			o_jbe = 6,
			o_jna = 6,
			o_jnbe = 7,
			o_ja = 7,
			o_js = 8,
			o_jns = 9,
			o_jp = 10,
			o_jpe = 10,
			o_jnp = 11,
			o_jpo = 11,
			o_jl = 12,
			o_jnge = 12,
			o_jnl = 13,
			o_jge = 13,
			o_jle = 14,
			o_jng = 14,
			o_jnle = 15,
			o_jg = 15,
		};
	}

	template <typename src_t, typename dst_t> void EMIT_ALU_OP(uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_ALU_OP(uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_BIT_OP(uint8_t*& code_buffer, X86OpType::EX86BitOpType optype, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_MOV_OP(uint8_t*& code_buffer, src_t src, dst_t dst);
	template <typename dst_t> uint8_t* EMIT_RIP_MOV_OP(uint8_t*& code_buffer, dst_t dst);
	template <typename constant_t> void EMIT_RIP_CONSTANT(uint8_t*& code_buffer, uint8_t * rip_offset, constant_t constant);
	template <typename src_t, typename dst_t> void EMIT_LEA_OP(uint8_t*& code_buffer, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_MOVZX_OP(uint8_t*& code_buffer, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_MUL_OP(uint8_t*& code_buffer, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_BSR_OP(uint8_t*& code_buffer, src_t src, dst_t dst);
	template <typename src_t, typename dst_t> void EMIT_BSF_OP(uint8_t*& code_buffer, src_t src, dst_t dst);
	template <typename op_t, bool push> void EMIT_STK_OP(uint8_t*& code_buffer, op_t src);
	template <typename op_t> void EMIT_SET(uint8_t*& code_buffer, X86OpType::EX86JmpOpType jmp_type, op_t src);
}

#include "instructions.h"

namespace x86
{
	using namespace insn;

	static inline uint32_t *EMIT_J(uint8_t *&code_buffer, X86OpType::EX86JmpOpType jmp_type)
	{
		E(0x0f);
		E(0x80 + (uint8_t)jmp_type);
		uint8_t *c = code_buffer;
		EMIT_CONSTANT<4>(code_buffer, 0);

		return (uint32_t*)c;
	}

	static inline void EMIT_J(uint8_t *&code_buffer, X86OpType::EX86JmpOpType jmp_type, int32_t rel)
	{
		E(0x0f);
		E(0x80 + (uint8_t)jmp_type);
		EMIT_CONSTANT<4>(code_buffer, rel);
	}

	static inline uint8_t *EMIT_WEE_J(uint8_t *&code_buffer, X86OpType::EX86JmpOpType jmp_type, int8_t rel)
	{
		uint8_t *p;
		E(0x70 + (uint8_t)jmp_type);
		p = code_buffer;

		EMIT_CONSTANT<1>(code_buffer, rel);
		return p;
	}

	template<> inline void EMIT_SET<RegId>(uint8_t *&code_buffer, X86OpType::EX86JmpOpType jmp_type, RegId op)
	{
		assert(RS(op) == 1);

		if (NEWREG(op))
			E(0x40);
		else if (HR(op))
			EMIT_REX(code_buffer, false, false, true);
		E(0x0f);
		E(0x90 + (uint8_t)jmp_type);

		EMIT_MODRM(code_buffer, 3, 0, RN(op));
	}

	inline void EMIT_WEE_JMP(uint8_t *&code_buffer, int8_t rel)
	{
		E(0xeb);
		EMIT_CONSTANT<1>(code_buffer, rel);
	}

	inline void EMIT_JMP(uint8_t *&code_buffer, int32_t rel)
	{
		E(0xe9);
		EMIT_CONSTANT<4>(code_buffer, rel);
	}

	inline void EMIT_IND_JMP(uint8_t *&code_buffer, RegId reg)
	{
		assert(!HR(reg));
		assert(RS(reg) == 8);

		E(0xff);
		EMIT_MODRM(code_buffer, 3, 4, RN(reg));
	}

	inline void EMIT_IND_CALL(uint8_t *&code_buffer, RegId reg)
	{
		assert(!HR(reg));
		assert(RS(reg) == 8);

		E(0xff);
		EMIT_MODRM(code_buffer, 3, 2, RN(reg));
	}

	static inline void EMIT_IMM_MEM_OP(uint8_t *&code_buffer, uint16_t op, uint8_t opc, uint8_t imm_size_bytes, const MemAccessInfo& mem)
	{
		uint8_t mod;

		assert(mem.base != RegRSP);
		assert(mem.base != RegR12B);
		assert(mem.base != RegR12W);
		assert(mem.base != RegR12D);
		assert(mem.base != RegR12);

		// --- Emit any prefixes
		if (RS(mem.base) == 4) E(0x67);
		if (imm_size_bytes == 2) E(0x66);

		bool b = false, x = false, r = false, w = false;

		if (imm_size_bytes == 8)
			w = true;

		if (HR(mem.base))
			b = true;

		if (mem.scale != 0 && HR(mem.index))
			x = true;

		if (b || x || r || w)
			EMIT_REX_BYTE(code_buffer, b, x, r, w);

		// --- Emit the opcode
		if (op > 0xff) {
			E(0x0f);
		}

		E(op & 0xff);

		// --- Now do MODRM SIB stuff

		// If there is no displacement, and the base register is not RBP, we can use an EADDR
		// encoding.  Alternatively, if we're trying to do a PC-relative operation, we *must* use the EADDR encoding.
		if ((mem.displ == 0 && mem.base != RegRBP && mem.base != RegR13) || mem.base == RegRIP) mod = 0;
		// If the displacement fits in an 8-bit integer, we can use a DISP8 encoding.
		else if ((mem.displ > INT8_MIN) && (mem.displ < INT8_MAX)) mod = 1;
		// Otherwise, we need to use a DISP32 encoding.
		else mod = 2;

		// Initially assume the RM field is simply the base register.
		uint8_t rm = RN(mem.base);

		// If we want a scale, then we need to use a SIB byte, which means we override the RM field.
		if (mem.scale != 0) rm = 4;

		// We have enough information to emit the MODRM now.
		EMIT_MODRM(code_buffer, mod, opc, rm);

		// If we're emitting a SIB byte...
		if (rm == 4) {
			uint8_t ss = 0;
			switch(mem.scale) {
				case 1:
					ss = 0;
					break;
				case 2:
					ss = 1;
					break;
				case 4:
					ss = 2;
					break;
				case 8:
					ss = 3;
					break;
				default:
					assert(false && "Invalid scale");
			}

			uint8_t index = RN(mem.index);
			uint8_t base = RN(mem.base);

			if (mod == 0)
				assert(base != 5);

			EMIT_SIB(code_buffer, ss, index, base);
		}

		// Emit an 8-bit displacement.
		if (mod == 1) EMIT_CONSTANT<1>(code_buffer, mem.displ);
		// Or, emit a 32-bit displacement.
		else if (mod == 2) EMIT_CONSTANT<4>(code_buffer, mem.displ);
	}

	template<> inline void EMIT_MOV_OP<uint8_t, RegId> (uint8_t*& code_buffer, uint8_t src, RegId dst)
	{
		assert(RS(dst) == 1);

		if (NEWREG(dst))
			EMIT_REX_BYTE(code_buffer, 0, 0, 0, 0);
		else
			EMIT_REX(code_buffer, 0, 0, HR(dst));

		uint8_t b = 0xb0;
		b += RN(dst);

		E(b);
		EMIT_CONSTANT<1>(code_buffer, src);
	}

	template<> inline void EMIT_MOV_OP<uint16_t, RegId> (uint8_t*& code_buffer, uint16_t src, RegId dst)
	{
		assert(RS(dst) == 2);
		E(0x66);
		EMIT_REX(code_buffer, 0, 0, HR(dst));

		uint8_t b = 0xb8;
		b += RN(dst);

		E(b);
		EMIT_CONSTANT<2>(code_buffer, src);
	}

	template<> inline void EMIT_MOV_OP<uint32_t, RegId> (uint8_t*& code_buffer, uint32_t src, RegId dst)
	{
		assert(RS(dst) == 4 || RS(dst) == 8);
		EMIT_REX(code_buffer, 0, 0, HR(dst));

		uint8_t b = 0xb8;
		b += RN(dst);

		E(b);
		EMIT_CONSTANT<4>(code_buffer, src);
	}

	template<> inline void EMIT_MOV_OP<uint64_t, RegId> (uint8_t*& code_buffer, uint64_t src, RegId dst)
	{
		assert(RS(dst) == 8);
		uint8_t b = 0xb8;
		b += RN(dst);

		if (src < UINT32_MAX) {
			EMIT_REX(code_buffer, 0, 0, HR(dst));
			E(b);
			EMIT_CONSTANT<4>(code_buffer, (uint32_t)src);
		} else {
			EMIT_REX(code_buffer, 1, 0, HR(dst));
			E(b);
			EMIT_CONSTANT<8>(code_buffer, src);
		}
	}

	template<> inline void EMIT_MOV_OP<RegId, RegId> (uint8_t*& code_buffer, RegId src, RegId dst)
	{
		uint16_t op = 0x8a;
		if (RS(dst) != 1)
			op |= 1;

		if (NEWREG(src)) {
			op = 0x88;
			insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(op, src, dst, insn::RegToRM));
		} else {
			insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(op, src, dst, insn::RMToReg));
		}
	}

	template<> inline void EMIT_MOV_OP<RegId, MemAccessInfo> (uint8_t*& code_buffer, RegId src, const MemAccessInfo dst)
	{
		uint16_t op = 0x88;
		if (RS(src) != 1)
			op |= 0x1;

		insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, MemAccessInfo>::get(op, src, dst, insn::RegToRM));
	}

	template<> inline void EMIT_MOV_OP<MemAccessInfo, RegId> (uint8_t*& code_buffer, const MemAccessInfo src, RegId dst)
	{
		uint16_t op = 0x8a;
		if (RS(dst) != 1)
			op |= 0x1;

		insn::EMIT(code_buffer, insn::X86InstructionModRM<MemAccessInfo, RegId>::get(op, src, dst, insn::RMToReg));
	}

	template<> inline void EMIT_MOV_OP<uint8_t, MemAccessInfo> (uint8_t*& code_buffer, uint8_t src, const MemAccessInfo dst)
	{
		uint16_t op = 0xc6;

		EMIT_IMM_MEM_OP(code_buffer, op, 0, 1, dst);
		EMIT_CONSTANT<1>(code_buffer, src);
	}

	template<> inline void EMIT_MOV_OP<uint16_t, MemAccessInfo> (uint8_t*& code_buffer, uint16_t src, const MemAccessInfo dst)
	{
		uint16_t op = 0xc7;

		EMIT_IMM_MEM_OP(code_buffer, op, 0, 2, dst);
		EMIT_CONSTANT<2>(code_buffer, src);
	}

	template<> inline void EMIT_MOV_OP<uint32_t, MemAccessInfo> (uint8_t*& code_buffer, uint32_t src, const MemAccessInfo dst)
	{
		uint16_t op = 0xc7;

		EMIT_IMM_MEM_OP(code_buffer, op, 0, 4, dst);
		EMIT_CONSTANT<4>(code_buffer, src);
	}

	template<> inline uint8_t* EMIT_RIP_MOV_OP<RegId>(uint8_t *&code_buffer, RegId dst)
	{
		EMIT_REX(code_buffer, RS(dst) == 8, false, HR(dst));

		uint8_t op = 0x8a;
		if(RS(dst) != 1) op |= 1;

		EMIT_BYTE(code_buffer, op);
		EMIT_MODRM(code_buffer, 0, RN(dst), 5);

		uint8_t *rval = code_buffer;

		EMIT_BYTE(code_buffer, 0);
		EMIT_BYTE(code_buffer, 0);
		EMIT_BYTE(code_buffer, 0);
		EMIT_BYTE(code_buffer, 0);
		return rval;
	}

	template<> inline void EMIT_RIP_CONSTANT<uint64_t>(uint8_t*& code_buffer, uint8_t * target, uint64_t constant)
	{
		uint32_t *rip_offset = (uint32_t*) target;
		*rip_offset = ((uint32_t)(code_buffer - target)) - 4;
		EMIT_CONSTANT<8>(code_buffer, constant);
	}

	inline void EMIT_MOVSX_OP(uint8_t*& code_buffer, RegId src, RegId dst)
	{
		assert(RS(dst) > RS(src));

		EMIT_REX(code_buffer, RS(dst) == 8, HR(dst), HR(src));

		if (RS(src) == 1) {
			E(0x0f);
			E(0xbe);
		} else if (RS(src) == 2) {
			E(0x0f);
			E(0xbf);
		} else if (RS(src) == 4) {
			E(0x63);
		} else assert(false && "Invalid source register size");

		EMIT_MODRM(code_buffer, 3, RN(dst), RN(src));
	}

	inline void EMIT_MOVSX_MEM_OP(uint8_t*& code_buffer, int size, MemAccessInfo src, RegId dst)
	{
		assert(RS(dst) > size);

		//EMIT_REX(code_buffer, RS(dst) == 8, HR(src.base), HR(dst));

		uint16_t op = 0x100;

		if (size == 1)
			op = 0x1be;
		else if (size == 2)
			op = 0x1bf;
		else if (size == 4)
			op = 0x63;
		else assert(false && "Invalid source register size");

		EMIT_MEM(code_buffer, op, dst, src);
	}

	inline void EMIT_MOVZX_OP(uint8_t*& code_buffer, RegId src, RegId dst)
	{
		assert(RS(dst) > RS(src));

		if (RS(src) == 1) {
			if(NEWREG(src) || HR(src) || HR(dst) || RS(dst)==8) EMIT_REX_BYTE(code_buffer,HR(src), 0, HR(dst), RS(dst)==8);

			E(0x0f);
			E(0xb6);
		} else if (RS(src) == 2) {
			EMIT_REX(code_buffer, RS(dst) == 8, HR(src), HR(dst));
			E(0x0f);
			E(0xb7);
		} else if (RS(src) == 4) {
			EMIT_REX(code_buffer, RS(dst) == 8, HR(src), HR(dst));
			E(0x63);
		} else assert(false && "Invalid source register size");

		EMIT_MODRM(code_buffer, 3, RN(dst), RN(src));
	}

	inline void EMIT_MOVZX_MEM_OP(uint8_t*& code_buffer, int size, const MemAccessInfo src, RegId dst)
	{
		assert(size != 4);
		assert(RS(dst) > size);

		EMIT_REX(code_buffer, RS(dst) == 8, HR(src.base), HR(dst));

		uint16_t op = 0x100;

		if (size == 1)
			op = 0x1b6;
		else if (size == 2)
			op = 0x1b7;
		else assert(false && "Invalid source register size");

		EMIT_MEM(code_buffer, op, dst, src);
	}

	template<> inline void EMIT_LEA_OP<MemAccessInfo, RegId> (uint8_t*& code_buffer, const MemAccessInfo src, RegId dst)
	{
		uint16_t op = 0x8d;
		if (RS(dst) != 1)
			op |= 0x1;

		insn::EMIT(code_buffer, insn::X86InstructionModRM<MemAccessInfo, RegId>::get(op, src, dst, insn::RMToReg));
	}

	template<> inline void EMIT_ALU_OP<RegId, MemAccessInfo> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, RegId src, const MemAccessInfo dst)
	{
		uint16_t op = 0x0;
		op |= ((uint16_t)optype) << 3;

		if (RS(src) != 1)
			op |= 0x1;

		insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, MemAccessInfo>::get(op, src, dst, insn::RegToRM));
	}

	template<> inline void EMIT_ALU_OP<uint32_t, MemAccessInfo> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, uint32_t src, const MemAccessInfo dst)
	{
		uint16_t op = 0x81;

		EMIT_IMM_MEM_OP(code_buffer, op, (uint8_t)optype, 4, dst);
		EMIT_CONSTANT<4>(code_buffer, src);
	}

	template<> inline void EMIT_ALU_OP<uint16_t, MemAccessInfo> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, uint16_t src, const MemAccessInfo dst)
	{
		uint16_t op = 0x81;

		EMIT_IMM_MEM_OP(code_buffer, op, (uint8_t)optype, 2, dst);
		EMIT_CONSTANT<2>(code_buffer, src);
	}

	template<> inline void EMIT_ALU_OP<RegId, RegId> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, RegId src, RegId dst)
	{
		assert(RS(dst) == RS(src));

		if (NEWREG(dst)) {
			uint16_t op = 0x2;
			op |= ((uint16_t)optype) << 3;

			insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(op, src, dst, insn::RMToReg));
		} else {
			uint16_t op = 0x0;
			op |= ((uint16_t)optype) << 3;

			if (RS(src) != 1)
				op |= 0x1;

			insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(op, src, dst, insn::RegToRM));
		}
	}

	template<> inline void EMIT_ALU_OP<MemAccessInfo, RegId> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, const MemAccessInfo src, RegId dst)
	{
		uint16_t op = 0x2;
		op |= ((uint16_t)optype) << 3;

		if (RS(dst) != 1)
			op |= 0x1;

		insn::EMIT(code_buffer, insn::X86InstructionModRM<MemAccessInfo, RegId>::get(op, src, dst, insn::RMToReg));
	}

	template<> inline void EMIT_ALU_OP<uint8_t, RegId> (uint8_t *&code_buffer,  X86OpType::EX86AluOpType optype, uint8_t src, RegId dst)
	{
		if (RS(dst) == 1) {
			if (NEWREG(dst))
				E(0x40);
			else
				EMIT_REX(code_buffer, false, false, HR(dst));

			E(0x80);
			EMIT_MODRM(code_buffer, 3, (uint8_t)optype, RN(dst));
			EMIT_CONSTANT<1>(code_buffer, src);
		} else {
			uint16_t op = 0x83;
			uint8_t opc = (uint8_t)optype;

			if (RS(dst) != 1)
				op |= 0x1;

			if (RS(dst) == 2)
				E(0x66);

			EMIT_REX(code_buffer, RS(dst) == 8, false, HR(dst));

			E(op);
			EMIT_MODRM(code_buffer, 3, opc, RN(dst));

			EMIT_CONSTANT<1>(code_buffer, src);
		}
	}

	template<> inline void EMIT_ALU_OP<uint8_t, MemAccessInfo> (uint8_t *&code_buffer,  X86OpType::EX86AluOpType optype, uint8_t src, MemAccessInfo dst)
	{
		uint16_t op = 0x80;

		EMIT_IMM_MEM_OP(code_buffer, op, (uint8_t)optype, 1, dst);
		EMIT_CONSTANT<1>(code_buffer, src);
	}

	template<> inline void EMIT_ALU_OP<uint16_t, RegId> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, uint16_t src, RegId dst)
	{
		assert(RS(dst) == 2);

		uint16_t op = 0x81;
		uint8_t opc = (uint8_t)optype;

		E(0x66);

		EMIT_REX(code_buffer, false, false, HR(dst));

		E(op);
		EMIT_MODRM(code_buffer, 3, opc, RN(dst));
		EMIT_CONSTANT<2>(code_buffer, src);
	}

	template<> inline void EMIT_ALU_OP<uint32_t, RegId> (uint8_t*& code_buffer, X86OpType::EX86AluOpType optype, uint32_t src, RegId dst)
	{
		assert(RS(dst) == 4 || RS(dst) == 8);

//	if(src >= INT32_MAX) {
//		EMIT_ALU_OP(code_buffer, optype, (uint64_t) src, dst);
//		return;
//	}

		if (src < INT8_MAX) {
			EMIT_ALU_OP(code_buffer, optype, (uint8_t)(src), dst);
			return;
		}

		uint16_t op = 0x81;
		uint8_t opc = (uint8_t)optype;

		EMIT_REX(code_buffer, RS(dst) == 8, false, HR(dst));

		E(op);
		EMIT_MODRM(code_buffer, 3, opc, RN(dst));
		EMIT_CONSTANT<4>(code_buffer, src);
	}

	template<> inline void EMIT_BIT_OP<uint8_t, RegId> (uint8_t*& code_buffer, X86OpType::EX86BitOpType optype, uint8_t src, RegId dst)
	{
		uint8_t op = 0xc0;

		if (RS(dst) != 1)
			op |= 0x1;

		EMIT_REX(code_buffer, RS(dst) == 8, 0, HR(dst));

		E(op);
		EMIT_MODRM(code_buffer, 3, (uint8_t)optype, RN(dst));
		E(src);
	}

	inline void EMIT_BIT_OP_MEM(uint8_t*& code_buffer, X86OpType::EX86BitOpType optype, uint8_t src, MemAccessInfo dst, uint8_t width)
	{
		uint16_t op = 0xc0;

		if (width != 1)
			op |= 0x1;

		EMIT_REX(code_buffer, width== 8, 0, 0);

		EMIT_IMM_MEM_OP(code_buffer, op, (uint8_t)optype, 1, dst);
		E(src);
	}

	inline void EMIT_BIT_OP_MEM(uint8_t*& code_buffer, X86OpType::EX86BitOpType optype, RegId src, MemAccessInfo dst, uint8_t width)
	{
		assert(src == RegCL);

		uint16_t op = 0xd2;

		if (width != 1)
			op |= 0x1;

		EMIT_IMM_MEM_OP(code_buffer, op, (uint8_t)optype, width, dst);
	}

	template<> inline void EMIT_BIT_OP<RegId, RegId> (uint8_t*& code_buffer, X86OpType::EX86BitOpType optype, RegId src, RegId dst)
	{
		assert(src == RegCL);
		uint8_t op = 0xd2;

		if (RS(dst) != 1)
			op |= 0x1;

		EMIT_REX(code_buffer, RS(dst) == 8, 0, HR(dst));

		E(op);
		EMIT_MODRM(code_buffer, 3, (uint8_t)optype, RN(dst));
	}

	template<> inline void EMIT_MUL_OP<RegId, RegId> (uint8_t*& code_buffer, RegId src, RegId dst)
	{
		assert(RS(src) == RS(dst));
		insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(0x1af, src, dst, insn::RMToReg));
	}

	template<> inline void EMIT_MUL_OP<MemAccessInfo, RegId> (uint8_t*& code_buffer, MemAccessInfo src, RegId dst)
	{
		E(0x0f);
		E(0xaf);

		assert(false);
	}

	template<> inline void EMIT_BSR_OP<RegId, RegId> (uint8_t *& code_buffer, RegId src, RegId dst)
	{
		assert(RS(src) == RS(dst));
		assert(RS(src) != 1);
		insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(0x1bd, src, dst, insn::RMToReg));
	}

	template<> inline void EMIT_BSF_OP<RegId, RegId> (uint8_t *& code_buffer, RegId src, RegId dst)
	{
		assert(RS(src) == RS(dst));
		assert(RS(src) != 1);
		insn::EMIT(code_buffer, insn::X86InstructionModRM<RegId, RegId>::get(0x1bc, src, dst, insn::RMToReg));
	}

	template<> inline void EMIT_STK_OP<RegId, true> (uint8_t*& code_buffer, RegId op)
	{
		assert(RS(op) == 8 || RS(op) == 2);
		if (RS(op) == 2)
			E(0x66);
		EMIT_REX(code_buffer, 0, 0, HR(op));
		E(0x50 + RN(op));
	}

	template<> inline void EMIT_STK_OP<uint64_t, true> (uint8_t*& code_buffer, uint64_t op)
	{
		assert(false);
	}

	template<> inline void EMIT_STK_OP<RegId, false> (uint8_t*& code_buffer, RegId op)
	{
		assert(RS(op) == 8 || RS(op) == 2);
		if (RS(op) == 2)
			E(0x66);
		EMIT_REX(code_buffer, 0, 0, HR(op));
		E(0x58 + RN(op));
	}

	inline void EMIT_CMOV_OP(uint8_t*& code_buffer, X86OpType::EX86JmpOpType cond, RegId src, RegId dst)
	{
		EMIT_REX(code_buffer, RS(dst) == 8, HR(src), HR(dst));
		E(0x0f);

		switch(cond) {
			case X86OpType::o_je:
				E(0x44);
				break;
			case X86OpType::o_jne:
				E(0x45);
				break;
			default:
				assert(false);
		}

		EMIT_MODRM(code_buffer, 3, RN(dst), RN(src));
	}

	inline void EMIT_CMOV_OP(uint8_t*& code_buffer, X86OpType::EX86JmpOpType cond, MemAccessInfo src, RegId dst)
	{
		assert(false);
	}

#undef E
}

#undef X86_MACROS_INTERNAL

#endif /* MACROS_H_ */
