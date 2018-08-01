/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   X86Decoder.h
 * Author: harry
 *
 * Created on 09 July 2018, 15:50
 */

#ifndef X86DECODER_H
#define X86DECODER_H

#include "abi/Address.h"
#include "core/MemoryInterface.h"
#include "gensim/gensim_decode.h"

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

enum X86Opcodes {
	INST_x86_add,
	INST_x86_and,
	INST_x86_bsf,
	INST_x86_bsr,
	INST_x86_bt,
	INST_x86_bts,
	INST_x86_call,
	INST_x86_cbw,
	INST_x86_cdq,
	INST_x86_cdqe,
	INST_x86_cld,
	INST_x86_clflush,

	INST_x86_cmov,

	INST_x86_cmp,
	INST_x86_cmpxchg,
	INST_x86_cpuid,
	INST_x86_cqo,
	INST_x86_cwd,
	INST_x86_cwde,
	INST_x86_dec,
	INST_x86_div,
	INST_x86_idiv,
	INST_x86_imul1,
	INST_x86_imul2,
	INST_x86_imul3,
	INST_x86_inc,

	INST_x86_jcond,

	INST_x86_jmp,
	INST_x86_lea,
	INST_x86_leave,
	INST_x86_mov,
	INST_x86_movd,
	INST_x86_movdqu,
	INST_x86_movq,
	INST_x86_movlpd,
	INST_x86_movhpd,
	INST_x86_movsb,
	INST_x86_movsxd,
	INST_x86_movsx,
	INST_x86_movups,
	INST_x86_movzx,
	INST_x86_mul,
	INST_x86_neg,
	INST_x86_nop,
	INST_x86_not,
	INST_x86_or,
	INST_x86_paddd,
	INST_x86_paddq,
	INST_x86_pand,
	INST_x86_pcmpeqb,
	INST_x86_pcmpeqd,
	INST_x86_pcmpgtb,
	INST_x86_pcmpgtw,
	INST_x86_pcmpgtd,
	INST_x86_pminub,
	INST_x86_pmovmskb,
	INST_x86_pop,
	INST_x86_popf,
	INST_x86_por,
	INST_x86_psubb,
	INST_x86_pslldq,
	INST_x86_psrldq,
	INST_x86_punpcklbw,
	INST_x86_punpcklwd,
	INST_x86_punpckldq,
	INST_x86_punpckhwd,
	INST_x86_punpckhdq,
	INST_x86_punpckhqdq,
	INST_x86_punpcklqdq,
	INST_x86_push,
	INST_x86_pushfq,
	INST_x86_pshufd,
	INST_x86_pslld,
	INST_x86_psllq,
	INST_x86_pxor,
	INST_x86_rdtsc,
	INST_x86_repe_cmpsb,
	INST_x86_rep_movsb,
	INST_x86_rep_movsd,
	INST_x86_rep_movsq,
	INST_x86_rep_stosb,
	INST_x86_rep_stosd,
	INST_x86_rep_stosq,
	INST_x86_repne_scasb,
	INST_x86_ret,
	INST_x86_rol,
	INST_x86_ror,
	INST_x86_sar,
	INST_x86_sbb,
	INST_x86_setcc,
	INST_x86_shl,
	INST_x86_shr,
	INST_x86_sub,
	INST_x86_syscall,
	INST_x86_test,
	INST_x86_xadd,
	INST_x86_xchg,
	INST_x86_xgetbv,
	INST_x86_xor
};

namespace archsim
{
	namespace arch
	{
		namespace x86
		{
			class X86Decoder : public gensim::BaseDecode
			{
			public:

				using Instruction = X86Decoder;

				struct Register {
					uint8_t index;
					uint8_t width;
					uint8_t regclass;

					// is this an 'h' reg (ah etc)
					uint8_t h_reg;

					// If this register is RIP, we need to offset the value by
					// the size of the instruction.
					uint32_t offset;
				};

				struct Memory {
					uint8_t has_segment;
					uint8_t segment;

					Register base;
					Register index;

					uint8_t width;
					uint8_t scale;
					uint64_t displacement;
				};

				struct Immediate {
					uint64_t value;
				};

				struct Operand {
					uint8_t is_reg;
					uint8_t is_imm;
					uint8_t is_mem;
					uint8_t is_relbr;

					union {
						Register reg;
						Memory memory;
						Immediate imm;
					};
				};


				Operand op0, op1, op2;
				uint8_t condition;

				X86Decoder();
				int DecodeInstr(Address addr, int mode,  MemoryInterface &interface);

			private:
				// These take pointers to xed_decoded_inst_t, but I don't want
				// XED to be part of the interface to the decoder. Forward
				// declarations are a bit of a nightmare because the decoded
				// inst struct is typedef'd. So, void* for now.
				bool DecodeOperands(void *inst);
				bool DecodeFlow(void *inst);
				bool DecodeClass(void *inst);
			};
		}
	}
}

#endif /* X86DECODER_H */

