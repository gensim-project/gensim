/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   IRBuilder.h
 * Author: harry
 *
 * Created on 29 March 2018, 16:41
 */

#ifndef IRBUILDER_H
#define IRBUILDER_H

#include <cassert>

namespace captive {
	namespace shared {
		
		class IRBuilder {
		public:
			IRBuilder() : current_block_(NOP_BLOCK), ctx_(nullptr) {}
			
			void SetContext(captive::arch::jit::TranslationContext *c) { ctx_ = c;}
			void SetBlock(IRBlockId block) { current_block_ = block; }
			
			void add_instruction(const IRInstruction &inst) {
				auto &insn = next_insn();
				insn = inst;
				insn.ir_block = current_block_;
			}
			
			IRRegId alloc_reg(int size) {
				return ctx_->alloc_reg(size);
			}
			
			IRBlockId alloc_block() {
				return ctx_->alloc_block();
			}
			IRBlockId GetBlock() const {
				assert(current_block_ != NOP_BLOCK);
				return current_block_;
			}
			
#define INSN0(x) void x() { auto &insn = next_insn(); insn = IRInstruction::x(); insn.ir_block = GetBlock(); }
#define INSN1(x) void x(const IROperand &op1) { auto &insn = next_insn(); insn = IRInstruction::x(op1); insn.ir_block = GetBlock(); }
#define INSN2(x) void x(const IROperand &op1, const IROperand &op2) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2); insn.ir_block = GetBlock(); }
#define INSN3(x) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2, op3); insn.ir_block = GetBlock(); }
#define INSN4(x) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2, op3, op4); insn.ir_block = GetBlock(); }
#define INSN5(x) void x(const IROperand &op1, const IROperand &op2, const IROperand &op3, const IROperand &op4, const IROperand &op5) { auto &insn = next_insn(); insn = IRInstruction::x(op1, op2, op3, op4, op5); insn.ir_block = GetBlock(); }
			
			INSN0(nop);
			INSN0(ret);
			INSN0(trap);
			
			INSN2(mov);
			INSN2(sx);
			INSN2(zx);
			INSN2(trunc);
			
			INSN3(cmov);
			
			INSN2(count);
			INSN2(profile);
			INSN1(verify);
			INSN1(ldpc);
			
			INSN2(add);
			INSN2(sub);
			INSN2(imul);
			INSN2(sdiv);
			INSN2(udiv);
			INSN2(mul);
			INSN2(mod);
			INSN2(shl);
			INSN2(shr);
			INSN2(sar);
			INSN2(ror);
			INSN2(clz);
			INSN2(bitwise_and);
			INSN2(bitwise_or);
			INSN2(bitwise_xor);
			INSN3(adc_with_flags);
			
			INSN4(vaddi);
			INSN4(vaddf);
			INSN4(vsubi);
			INSN4(vsubf);
			INSN4(vmulf);
			
			INSN1(fctrl_set_round);
			INSN1(fctrl_get_round);
			INSN1(fctrl_set_flush);
			INSN1(fctrl_get_flush);
			INSN3(fcmp_eq);
			INSN3(fcmp_lt);
			INSN3(fcmp_gt);
			INSN2(fcvt_single_to_double);
			INSN2(fcvt_double_to_single);
			INSN2(fcvt_float_to_si);
			INSN2(fcvtt_float_to_si);
			INSN2(fcvt_float_to_ui);
			INSN2(fcvtt_float_to_ui);
			INSN2(fcvt_si_to_float);
			INSN2(fcvt_ui_to_float);
			INSN3(fdiv);
			INSN3(fadd);
			INSN3(fsub);
			INSN3(fmul);
			INSN2(fsqrt);
			
			INSN3(read_device);
			INSN3(write_device);
			
			INSN1(updatezn);
			INSN3(cmpeq);
			INSN3(cmpne);
			INSN3(cmplt);
			INSN3(cmpslt);
			INSN3(cmplte);
			INSN3(cmpgt);
			INSN3(cmpsgt);
			INSN3(cmpgte);
			INSN3(cmpsgte);
			
			INSN2(streg);
			INSN2(ldreg);
			INSN2(ldmem);
			INSN2(stmem);
			INSN2(ldmem_user);
			INSN2(stmem_user);
			
			INSN1(jump);
			INSN3(branch);
			
			INSN0(flush_itlb);
			INSN1(flush_itlb_entry);
			INSN1(flush_dtlb_entry);
			INSN2(take_exception);
			
			void increment_pc(uint32_t amt) { auto &insn = next_insn(); insn = IRInstruction::incpc(IROperand::const32(amt)); insn.ir_block = current_block_; }
			
			INSN1(call);
			INSN2(call);
			INSN3(call);
			INSN4(call);
			INSN5(call);

			INSN4(dispatch);
			INSN2(set_cpu_feature);
			INSN1(set_cpu_mode);
			
#undef INSN0
#undef INSN1
#undef INSN2
#undef INSN3
#undef INSN4
			
		private:
			IRBlockId current_block_;
			captive::arch::jit::TranslationContext *ctx_;
			IRInstruction &next_insn() { IRInstruction &next_insn = ctx_->get_next_instruction(); next_insn.ir_block = current_block_; return next_insn; }
			
		};
		
	}
}

#endif /* IRBUILDER_H */

