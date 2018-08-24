/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   IRBuilder.h
 * Author: harry
 *
 * Created on 29 March 2018, 16:41
 */

#ifndef IRBUILDER_H
#define IRBUILDER_H

#include <cassert>

namespace captive
{
	namespace shared
	{

		class IRBuilder
		{
		public:
			IRBuilder() : current_block_(NOP_BLOCK), ctx_(nullptr) {}

			void SetContext(captive::arch::jit::TranslationContext *c)
			{
				ctx_ = c;
			}
			void SetBlock(IRBlockId block)
			{
				current_block_ = block;
			}

			void add_instruction(const IRInstruction &inst)
			{
				auto *insn = ctx_->get_next_instruction_ptr();
				new (insn) IRInstruction(inst);
				insn->ir_block = current_block_;
			}

			IRRegId alloc_reg(int size)
			{
				return ctx_->alloc_reg(size);
			}

			IRBlockId alloc_block()
			{
				return ctx_->alloc_block();
			}
			IRBlockId GetBlock() const
			{
				assert(current_block_ != NOP_BLOCK);
				return current_block_;
			}

//#define INSN0(x) void x() { auto &insn = next_insn(); insn = IRInstruction::x(); insn.ir_block = GetBlock(); }
//#define INSN1(x) void x(const IROperand &op1) { auto &insn = next_insn(); insn = IRInstruction::x(op1); insn.ir_block = GetBlock(); }
//#define INSN2(x) void x(const IROperand &op1, const IROperand &op2) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2); insn.ir_block = GetBlock(); }
//#define INSN3(x) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2, op3); insn.ir_block = GetBlock(); }
//#define INSN4(x) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2, op3, op4); insn.ir_block = GetBlock(); }
//#define INSN5(x) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4, const IROperand &op5) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2, op3, op4, op5); insn.ir_block = GetBlock(); }

#define INSN0(x, y) void x() { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y);  insn->ir_block = GetBlock(); }
#define INSN1(x, y) void x(const IROperand &op1) { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y, op1);  insn->ir_block = GetBlock(); }
#define INSN2(x, y) void x(const IROperand &op1, const IROperand &op2) { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y, op1,op2);  insn->ir_block = GetBlock(); }
#define INSN3(x, y) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3) { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y, op1,op2,op3);  insn->ir_block = GetBlock(); }
#define INSN4(x, y) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4) { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y, op1,op2,op3,op4);  insn->ir_block = GetBlock(); }
#define INSN5(x, y) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4, const IROperand &op5) { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y, op1,op2,op3,op4,op5);  insn->ir_block = GetBlock(); }
#define INSN6(x, y) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4, const IROperand &op5, const IROperand &op6) { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y, op1,op2,op3,op4,op5,op6);  insn->ir_block = GetBlock(); }

			INSN0(nop, NOP);
			INSN0(ret, RET);
			INSN0(trap, TRAP);

			INSN2(mov, MOV);
			INSN2(sx, SX);
			INSN2(zx, ZX);
			INSN2(trunc, TRUNC);

			INSN3(cmov, CMOV);

			INSN2(count, COUNT);
//			INSN2(profile);
//			INSN1(verify);
			INSN1(ldpc, LDPC);

			INSN2(add, ADD);
			INSN2(sub, SUB);
			INSN2(imul, IMUL);
			INSN2(sdiv, SDIV);
			INSN2(udiv, UDIV);
			INSN2(mul, MUL);
			INSN2(mod, MOD);
			INSN2(shl, SHL);
			INSN2(shr, SHR);
			INSN2(sar, SAR);
			INSN2(ror, ROR);
			INSN2(rol, ROL);
			INSN2(clz, CLZ);
			INSN2(bswap, BSWAP);
			INSN2(popcnt, POPCNT);
			INSN2(bitwise_and, AND);
			INSN2(bitwise_or, OR);
			INSN2(bitwise_xor, XOR);
			INSN3(adc_with_flags, ADC_WITH_FLAGS);
			INSN3(sbc_with_flags, SBC_WITH_FLAGS);

			INSN4(vaddi, VADDI);
			INSN4(vaddf, VADDF);
			INSN4(vsubi, VSUBI);
			INSN4(vsubf, VSUBF);
			INSN4(vmulf, VMULF);
			INSN4(vori, VORI);
			INSN4(vandi, VANDI);
			INSN4(vxori, VXORI);
			INSN4(vcmpeqi, VCMPEQI);
			INSN4(vcmpgti, VCMPGTI);
			INSN4(vcmpgtei, VCMPGTEI);

			INSN1(fctrl_set_round, FCTRL_SET_ROUND);
			INSN1(fctrl_get_round, FCTRL_GET_ROUND);
			INSN1(fctrl_set_flush, FCTRL_SET_FLUSH_DENORMAL);
			INSN1(fctrl_get_flush, FCTRL_GET_FLUSH_DENORMAL);
			INSN3(fcmp_eq, FCMP_EQ);
			INSN3(fcmp_lt, FCMP_LT);
			INSN3(fcmp_gt, FCMP_GT);
			INSN2(fcvt_single_to_double, FCVT_S_TO_D);
			INSN2(fcvt_double_to_single, FCVT_D_TO_S);
			INSN2(fcvt_float_to_si, FCVT_F_TO_SI);
			INSN2(fcvtt_float_to_si, FCVTT_F_TO_SI);
			INSN2(fcvt_float_to_ui, FCVT_F_TO_UI);
			INSN2(fcvtt_float_to_ui, FCVTT_F_TO_UI);
			INSN2(fcvt_si_to_float, FCVT_SI_TO_F);
			INSN2(fcvt_ui_to_float, FCVT_UI_TO_F);
			INSN3(fdiv, FDIV);
			INSN3(fadd, FADD);
			INSN3(fsub, FSUB);
			INSN3(fmul, FMUL);
			INSN2(fsqrt, FSQRT);
			INSN2(fabs, FABS);

			INSN3(read_device, READ_DEVICE);
			INSN3(write_device, WRITE_DEVICE);

			INSN1(updatezn, SET_ZN_FLAGS);
			INSN3(cmpeq, CMPEQ);
			INSN3(cmpne, CMPNE);
			INSN3(cmplt, CMPLT);
			INSN3(cmpslt, CMPSLT);
			INSN3(cmplte, CMPLTE);
			INSN3(cmpgt, CMPGT);
			INSN3(cmpsgt, CMPSGT);
			INSN3(cmpgte, CMPGTE);
			INSN3(cmpsgte, CMPSGTE);

			INSN2(streg, WRITE_REG);
			INSN2(ldreg, READ_REG);
			INSN4(ldmem, READ_MEM);
			INSN4(stmem, WRITE_MEM);

			INSN1(jump, JMP);
			INSN3(branch, BRANCH);

			INSN0(flush_itlb, FLUSH_ITLB);
			INSN1(flush_itlb_entry, FLUSH_ITLB_ENTRY);
			INSN1(flush_dtlb_entry, FLUSH_DTLB_ENTRY);
			INSN2(take_exception, TAKE_EXCEPTION);

			void increment_pc(uint32_t amt)
			{
				auto &insn = next_insn();
				insn = IRInstruction::incpc(IROperand::const32(amt));
				insn.ir_block = current_block_;
			}

			INSN2(call, CALL);
			INSN3(call, CALL);
			INSN4(call, CALL);
			INSN5(call, CALL);
			INSN6(call, CALL);

			INSN4(dispatch, DISPATCH);
			INSN2(set_cpu_feature, SET_CPU_FEATURE);
			INSN1(set_cpu_mode, SET_CPU_MODE);

#undef INSN0
#undef INSN1
#undef INSN2
#undef INSN3
#undef INSN4

		private:
			IRBlockId current_block_;
			captive::arch::jit::TranslationContext *ctx_;
			IRInstruction &next_insn()
			{
				IRInstruction &next_insn = ctx_->get_next_instruction();
				next_insn.ir_block = current_block_;
				return next_insn;
			}

		};

	}
}

#endif /* IRBUILDER_H */

