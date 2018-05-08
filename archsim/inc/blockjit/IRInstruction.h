/*
 * IRInstruction.h
 *
 *  Created on: 30 Sep 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_IRINSTRUCTION_H_
#define INC_BLOCKJIT_IRINSTRUCTION_H_

#include "blockjit/ir.h"
#include "blockjit/IROperand.h"

#include <cassert>
#include <cstdint>


namespace captive
{
	namespace shared
	{

		struct insn_descriptor {
			const char *mnemonic;
			const char format[7];
			bool has_side_effects;
		};

		extern struct insn_descriptor insn_descriptors[];
		extern size_t num_descriptors;

		struct IRInstruction {
			enum IRInstructionType : uint8_t {
				INVALID,

				VERIFY,
				COUNT,
				INTCHECK,

				NOP,
				TRAP,

				MOV,
				CMOV,
				LDPC,
				INCPC,

				ADD,
				SUB,
				IMUL,
				MUL,
				UDIV,
				SDIV,
				MOD,

				SHL,
				SHR,
				SAR,
				ROR,
				CLZ,

				AND,
				OR,
				XOR,

				CMPEQ,
				CMPNE,
				CMPGT,
				CMPGTE,
				CMPLT,
				CMPLTE,

				SX,
				ZX,
				TRUNC,

				READ_REG,
				WRITE_REG,
				READ_MEM,
				WRITE_MEM,
				READ_MEM_USER,
				WRITE_MEM_USER,

				CALL,
				JMP,
				BRANCH,
				RET,
				DISPATCH,

				SET_CPU_MODE,
				SET_CPU_FEATURE,
				WRITE_DEVICE,
				READ_DEVICE,
				PROBE_DEVICE,

				FLUSH,
				FLUSH_ITLB,
				FLUSH_DTLB,
				FLUSH_ITLB_ENTRY,
				FLUSH_DTLB_ENTRY,

				ADC_WITH_FLAGS,
				SET_ZN_FLAGS,

				BARRIER,

				TAKE_EXCEPTION,
				PROFILE,

				CMPSGT,
				CMPSGTE,
				CMPSLT,
				CMPSLTE,

				FMUL,
				FDIV,
				FADD,
				FSUB,
				FSQRT,

				FCMP_LT,
				FCMP_LTE,
				FCMP_GT,
				FCMP_GTE,
				FCMP_EQ,
				FCMP_NE,

				FCVT_UI_TO_F,
				FCVT_F_TO_UI,
				FCVTT_F_TO_UI,
				FCVT_SI_TO_F,
				FCVT_F_TO_SI,
				FCVTT_F_TO_SI,

				FCVT_S_TO_D,
				FCVT_D_TO_S,

				FCTRL_SET_ROUND,
				FCTRL_GET_ROUND,
				FCTRL_SET_FLUSH_DENORMAL,
				FCTRL_GET_FLUSH_DENORMAL,

				VADDI,
				VADDF,
				VSUBI,
				VSUBF,
				VMULI,
				VMULF,

				_END
			};

			IRBlockId ir_block;
			IRInstructionType type;
			IROperand operands[6];

			IRInstruction(IRInstructionType type)
				: type(type),
				  operands { IROperand::none(), IROperand::none(), IROperand::none(), IROperand::none(), IROperand::none(), IROperand::none() } { }

			IRInstruction(IRInstructionType type, const IROperand& op1)
				: type(type),
				  operands { op1, IROperand::none(), IROperand::none(), IROperand::none(), IROperand::none(), IROperand::none() } { }

			IRInstruction(IRInstructionType type, const IROperand& op1, const IROperand& op2)
				: type(type),
				  operands { op1, op2, IROperand::none(), IROperand::none(), IROperand::none(), IROperand::none() } { }

			IRInstruction(IRInstructionType type, const IROperand& op1, const IROperand& op2, const IROperand& op3)
				: type(type),
				  operands { op1, op2, op3, IROperand::none(), IROperand::none(), IROperand::none() } { }

			IRInstruction(IRInstructionType type, const IROperand& op1, const IROperand& op2, const IROperand& op3, const IROperand& op4)
				: type(type),
				  operands { op1, op2, op3, op4, IROperand::none(), IROperand::none() } { }

			IRInstruction(IRInstructionType type, const IROperand& op1, const IROperand& op2, const IROperand& op3, const IROperand& op4, const IROperand& op5)
				: type(type),
				  operands { op1, op2, op3, op4, op5, IROperand::none() } { }

			IRInstruction(IRInstructionType type, const IROperand& op1, const IROperand& op2, const IROperand& op3, const IROperand& op4, const IROperand& op5, const IROperand& op6)
				: type(type),
				  operands { op1, op2, op3, op4, op5, op6 } { }

			uint8_t count_operands() const
			{
				int count = 0;
				for(int i = 0; i < 6; ++i) count += operands[i].type == IROperand::NONE;
				return count;
			}

			static IRInstruction nop()
			{
				return IRInstruction(NOP);
			}
			static IRInstruction ret()
			{
				return IRInstruction(RET);
			}
			static IRInstruction dispatch(const IROperand &target, const IROperand &fallthrough, const IROperand &target_block, const IROperand &fallthrough_block)
			{
				assert(target.is_constant() && fallthrough.is_constant());
				return IRInstruction(DISPATCH, target, fallthrough, target_block, fallthrough_block);
			}
			static IRInstruction trap()
			{
				return IRInstruction(TRAP);
			}
			static IRInstruction verify(const IROperand &pc)
			{
				return IRInstruction(VERIFY, pc);
			}
			static IRInstruction int_check()
			{
				return IRInstruction(INTCHECK);
			}

			static IRInstruction count(const IROperand &pointer, const IROperand &count)
			{
				return IRInstruction(COUNT, pointer, count);
			}
			static IRInstruction profile(const IROperand &pointer, const IROperand &block_offset)
			{
				return IRInstruction(PROFILE, pointer, block_offset);
			}

			static IRInstruction ldpc(const IROperand &dst)
			{
				assert(dst.is_vreg() && dst.size == 4);
				return IRInstruction(LDPC, dst);
			}
			static IRInstruction incpc(const IROperand &amt)
			{
				assert(amt.is_constant());
				return IRInstruction(INCPC, amt);
			}

			static IRInstruction flush()
			{
				return IRInstruction(FLUSH);
			}
			static IRInstruction flush_itlb()
			{
				return IRInstruction(FLUSH_ITLB);
			}
			static IRInstruction flush_dtlb()
			{
				return IRInstruction(FLUSH_DTLB);
			}
			static IRInstruction flush_itlb_entry(const IROperand &addr)
			{
				return IRInstruction(FLUSH_ITLB_ENTRY, addr);
			}
			static IRInstruction flush_dtlb_entry(const IROperand &addr)
			{
				return IRInstruction(FLUSH_DTLB_ENTRY, addr);
			}

			static IRInstruction adc_with_flags(const IROperand &lhs_in, const IROperand &rhs_in, const IROperand &carry_in)
			{
				return IRInstruction(ADC_WITH_FLAGS, lhs_in, rhs_in, carry_in);
			}
			static IRInstruction updatezn(const IROperand &val)
			{
				return IRInstruction(SET_ZN_FLAGS, val);
			}

			static IRInstruction barrier()
			{
				return IRInstruction(BARRIER);
			}
			static IRInstruction take_exception(const IROperand &type, const IROperand &data)
			{
				return IRInstruction(TAKE_EXCEPTION, type, data);
			}
			//
			// Data Motion
			//
			static IRInstruction mov(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(MOV, src, dst);
			}

			static IRInstruction cmov(const IROperand &cond, const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(CMOV, cond, src, dst);
			}

			static IRInstruction sx(const IROperand &src, const IROperand &dst)
			{
				assert(src.size < dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(SX, src, dst);
			}

			static IRInstruction zx(const IROperand &src, const IROperand &dst)
			{
				assert(src.size < dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(ZX, src, dst);
			}

			static IRInstruction trunc(const IROperand &src, const IROperand &dst)
			{
				assert(src.size > dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(TRUNC, src, dst);
			}

			// FP Operations
			static IRInstruction fdiv(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size && op2.size == dest.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FDIV, op1, op2, dest);
			}

			static IRInstruction fmul(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size && op2.size == dest.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FMUL, op1, op2, dest);
			}

			static IRInstruction fadd(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size && op2.size == dest.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FADD, op1, op2, dest);
			}

			static IRInstruction fsub(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size && op2.size == dest.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FSUB, op1, op2, dest);
			}

			static IRInstruction fsqrt(const IROperand &op1, const IROperand &dest)
			{
				assert(op1.size == dest.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FSQRT, op1, dest);
			}

			static IRInstruction fcmp_lt(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FCMP_LT, op1, op2, dest);
			}

			static IRInstruction fcmp_lte(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FCMP_LTE, op1, op2, dest);
			}

			static IRInstruction fcmp_gt(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FCMP_GT, op1, op2, dest);
			}

			static IRInstruction fcmp_gte(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FCMP_GTE, op1, op2, dest);
			}

			static IRInstruction fcmp_eq(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FCMP_EQ, op1, op2, dest);
			}

			static IRInstruction fcmp_ne(const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(op1.size == op2.size);
				assert(op1.size == 4 || op1.size == 8);
				assert(dest.is_vreg());

				return IRInstruction(FCMP_NE, op1, op2, dest);
			}

			static IRInstruction fcvt_si_to_float(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCVT_SI_TO_F, op1, dest);
			}

			static IRInstruction fcvt_float_to_si(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCVT_F_TO_SI, op1, dest);
			}

			static IRInstruction fcvtt_float_to_si(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCVTT_F_TO_SI, op1, dest);
			}

			static IRInstruction fcvt_ui_to_float(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCVT_UI_TO_F, op1, dest);
			}

			static IRInstruction fcvt_float_to_ui(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCVT_F_TO_UI, op1, dest);
			}

			static IRInstruction fcvtt_float_to_ui(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCVTT_F_TO_UI, op1, dest);
			}

			static IRInstruction fcvt_double_to_single(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				assert(op1.size == 8);
				assert(dest.size == 4);

				return IRInstruction(FCVT_D_TO_S, op1, dest);
			}

			static IRInstruction fcvt_single_to_double(const IROperand &op1, const IROperand &dest)
			{
				assert(dest.is_vreg());
				assert(op1.size == 4);
				assert(dest.size == 8);

				return IRInstruction(FCVT_S_TO_D, op1, dest);
			}

			static IRInstruction fctrl_set_round(const IROperand &op1)
			{
				assert(op1.size == 4);
				return IRInstruction(FCTRL_SET_ROUND, op1);
			}

			static IRInstruction fctrl_get_round(const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCTRL_GET_ROUND, dest);
			}

			static IRInstruction fctrl_set_flush(const IROperand &op1)
			{
				assert(op1.size == 4);
				return IRInstruction(FCTRL_SET_FLUSH_DENORMAL, op1);
			}

			static IRInstruction fctrl_get_flush(const IROperand &dest)
			{
				assert(dest.is_vreg());
				return IRInstruction(FCTRL_GET_FLUSH_DENORMAL, dest);
			}

			// Vector operations

			static IRInstruction vaddf(const IROperand &width, const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(width.is_constant());

				return IRInstruction(VADDF, width, op1, op2, dest);
			}

			static IRInstruction vaddi(const IROperand &width, const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(width.is_constant());

				return IRInstruction(VADDI, width, op1, op2, dest);
			}

			static IRInstruction vsubf(const IROperand &width, const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(width.is_constant());

				return IRInstruction(VSUBF, width, op1, op2, dest);
			}

			static IRInstruction vsubi(const IROperand &width, const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(width.is_constant());

				return IRInstruction(VSUBI, width, op1, op2, dest);
			}

			static IRInstruction vmulf(const IROperand &width, const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(width.is_constant());

				return IRInstruction(VMULF, width, op1, op2, dest);
			}

			static IRInstruction vmuli(const IROperand &width, const IROperand &op1, const IROperand &op2, const IROperand &dest)
			{
				assert(width.is_constant());

				return IRInstruction(VMULI, width, op1, op2, dest);
			}

			//
			// Arithmetic Operations
			//
			static IRInstruction add(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg() || src.is_pc());
				assert(dst.is_vreg());

				return IRInstruction(ADD, src, dst);
			}

			static IRInstruction sub(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(SUB, src, dst);
			}

			static IRInstruction mul(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(MUL, src, dst);
			}

			static IRInstruction imul(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(IMUL, src, dst);
			}

			static IRInstruction sdiv(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(SDIV, src, dst);
			}

			static IRInstruction udiv(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(UDIV, src, dst);
			}

			static IRInstruction mod(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(MOD, src, dst);
			}

			// Bit-shifting
			static IRInstruction shl(const IROperand &amt, const IROperand &dst)
			{
				assert(amt.is_constant() || amt.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(SHL, amt, dst);
			}

			static IRInstruction shr(const IROperand &amt, const IROperand &dst)
			{
				assert(amt.is_constant() || amt.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(SHR, amt, dst);
			}

			static IRInstruction sar(const IROperand &amt, const IROperand &dst)
			{
				assert(amt.is_constant() || amt.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(SAR, amt, dst);
			}

			static IRInstruction ror(const IROperand &amt, const IROperand &dst)
			{
				assert(amt.is_constant() || amt.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(ROR, amt, dst);
			}

			// Bit manipulation
			static IRInstruction clz(const IROperand &src, const IROperand &dst)
			{
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(CLZ, src, dst);
			}

			static IRInstruction bitwise_and(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(AND, src, dst);
			}

			static IRInstruction bitwise_or(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(OR, src, dst);
			}

			static IRInstruction bitwise_xor(const IROperand &src, const IROperand &dst)
			{
				assert(src.size == dst.size);
				assert(src.is_constant() || src.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(XOR, src, dst);
			}

			//
			// Comparison
			//
			static IRInstruction cmpeq(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPEQ, lh, rh, dst);
			}

			static IRInstruction cmpne(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPNE, lh, rh, dst);
			}

			static IRInstruction cmplt(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPLT, lh, rh, dst);
			}

			static IRInstruction cmplte(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPLTE, lh, rh, dst);
			}

			static IRInstruction cmpgt(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPGT, lh, rh, dst);
			}

			static IRInstruction cmpgte(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPGTE, lh, rh, dst);
			}

			static IRInstruction cmpslt(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPSLT, lh, rh, dst);
			}

			static IRInstruction cmpslte(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPSLTE, lh, rh, dst);
			}

			static IRInstruction cmpsgt(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPSGT, lh, rh, dst);
			}

			static IRInstruction cmpsgte(const IROperand &lh, const IROperand &rh, const IROperand &dst)
			{
				assert(lh.size == rh.size);
				assert(lh.is_vreg() || lh.is_constant());
				assert(rh.is_vreg() || rh.is_constant());
				assert(!(lh.is_constant() && rh.is_constant()));
				assert(dst.is_vreg());

				return IRInstruction(CMPSGTE, lh, rh, dst);
			}

			//
			// Domain-specific operations
			//
			static IRInstruction ldreg(const IROperand &offset, const IROperand &dst)
			{
				assert(offset.is_constant() || offset.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(READ_REG, offset, dst);
			}

			static IRInstruction streg(const IROperand &src, const IROperand &offset)
			{
				assert(offset.is_constant() || offset.is_vreg());
				assert(src.is_constant() || src.is_vreg());

				return IRInstruction(WRITE_REG, src, offset);
			}

			static IRInstruction ldmem(const IROperand &interface, const IROperand &offset, const IROperand &dst)
			{
				assert(interface.is_constant());
				assert(offset.is_constant() || offset.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(READ_MEM, interface, offset, IROperand::const32(0), dst);
			}

			static IRInstruction stmem(const IROperand &interface, const IROperand &src, const IROperand &offset)
			{
				assert(interface.is_constant());
				assert(offset.is_constant() || offset.is_vreg());
				assert(src.is_constant() || src.is_vreg());

				return IRInstruction(WRITE_MEM, interface, src, IROperand::const32(0), offset);
			}

			static IRInstruction ldmem_user(const IROperand &offset, const IROperand &dst)
			{
				assert(offset.is_constant() || offset.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(READ_MEM_USER, offset, dst);
			}

			static IRInstruction stmem_user(const IROperand &src, const IROperand &offset)
			{
				assert(offset.is_constant() || offset.is_vreg());
				assert(src.is_constant() || src.is_vreg());

				return IRInstruction(WRITE_MEM_USER, src, offset);
			}

			static IRInstruction write_device(const IROperand &dev, const IROperand &reg, const IROperand &val)
			{
				assert(dev.is_constant() || dev.is_vreg());
				assert(reg.is_constant() || reg.is_vreg());
				assert(val.is_constant() || val.is_vreg());

				return IRInstruction(WRITE_DEVICE, dev, reg, val);
			}

			static IRInstruction read_device(const IROperand &dev, const IROperand &reg, const IROperand &dst)
			{
				assert(dev.is_constant() || dev.is_vreg());
				assert(reg.is_constant() || reg.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(READ_DEVICE, dev, reg, dst);
			}

			static IRInstruction probe_device(const IROperand &dev, const IROperand &dst)
			{
				assert(dev.is_constant() || dev.is_vreg());
				assert(dst.is_vreg());

				return IRInstruction(PROBE_DEVICE, dev, dst);
			}

			static IRInstruction set_cpu_mode(const IROperand &mode)
			{
				assert(mode.is_constant() || mode.is_vreg());

				return IRInstruction(SET_CPU_MODE, mode);
			}

			static IRInstruction set_cpu_feature(const IROperand &feature, const IROperand &level)
			{
				assert(feature.is_constant() && level.is_constant());

				return IRInstruction(SET_CPU_FEATURE, feature, level);
			}

			//
			// Control Flow
			//
			static IRInstruction jump(const IROperand &target)
			{
				assert(target.is_block());
				return IRInstruction(JMP, target);
			}

			static IRInstruction branch(const IROperand &cond, const IROperand &tt, const IROperand &ft)
			{
				assert(cond.is_constant() || cond.is_vreg());
				assert(tt.is_block());
				assert(ft.is_block());

				return IRInstruction(BRANCH, cond, tt, ft);
			}

			static IRInstruction call(const IROperand &fn)
			{
				assert(fn.is_func());

				return IRInstruction(CALL, fn);
			}

			static IRInstruction call(const IROperand &fn, const IROperand &arg0)
			{
				assert(fn.is_func());
				assert(arg0.is_constant() || arg0.is_vreg());

				return IRInstruction(CALL, fn, arg0);
			}

			static IRInstruction call(const IROperand &fn, const IROperand &arg0, const IROperand &arg1)
			{
				assert(fn.is_func());
				assert(arg0.is_constant() || arg0.is_vreg());
				assert(arg1.is_constant() || arg1.is_vreg());

				return IRInstruction(CALL, fn, arg0, arg1);
			}

			static IRInstruction call(const IROperand &fn, const IROperand &arg0, const IROperand &arg1, const IROperand &arg2)
			{
				assert(fn.is_func());
				assert(arg0.is_constant() || arg0.is_vreg());
				assert(arg1.is_constant() || arg1.is_vreg());
				assert(arg2.is_constant() || arg2.is_vreg());

				return IRInstruction(CALL, fn, arg0, arg1, arg2);
			}

			static IRInstruction call(const IROperand &fn, const IROperand &arg0, const IROperand &arg1, const IROperand &arg2, const IROperand &arg3)
			{
				assert(fn.is_func());
				assert(arg0.is_constant() || arg0.is_vreg());
				assert(arg1.is_constant() || arg1.is_vreg());
				assert(arg2.is_constant() || arg2.is_vreg());
				assert(arg3.is_constant() || arg3.is_vreg());

				return IRInstruction(CALL, fn, arg0, arg1, arg2, arg3);
			}

			static IRInstruction call(const IROperand &fn, const IROperand &arg0, const IROperand &arg1, const IROperand &arg2, const IROperand &arg3, const IROperand &arg4)
			{
				assert(fn.is_func());
				assert(arg0.is_constant() || arg0.is_vreg());
				assert(arg1.is_constant() || arg1.is_vreg());
				assert(arg2.is_constant() || arg2.is_vreg());
				assert(arg3.is_constant() || arg3.is_vreg());
				assert(arg4.is_constant() || arg4.is_vreg());

				return IRInstruction(CALL, fn, arg0, arg1, arg2, arg3, arg4);
			}
		} __packed;
	}
}



#endif /* INC_BLOCKJIT_IRINSTRUCTION_H_ */
