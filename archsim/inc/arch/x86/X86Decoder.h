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
	INST_x86_call,

	INST_x86_cmov,

	INST_x86_cmp,
	INST_x86_cpuid,

	INST_x86_jcond,

	INST_x86_jmp,
	INST_x86_lea,
	INST_x86_mov,
	INST_x86_movsxd,
	INST_x86_movzx,
	INST_x86_nop,
	INST_x86_or,
	INST_x86_pop,
	INST_x86_push,
	INST_x86_ret,
	INST_x86_setz,
	INST_x86_sub,
	INST_x86_syscall,
	INST_x86_test,
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

				struct Register {
					uint8_t index;
					uint8_t width;

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

					Register reg;
					Memory memory;
					Immediate imm;
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
				void DecodeOperands(void *inst);
				void DecodeFlow(void *inst);
				void DecodeClass(void *inst);
			};
		}
	}
}

#endif /* X86DECODER_H */

