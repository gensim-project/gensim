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
	INST_x86_mov,
	INST_x86_pop,
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
				int DecodeInstr(Address addr, int mode,  MemoryInterface &interface);

				uint32_t op0_size;
				bool op0_is_reg;
				uint32_t op0_reg;

				uint32_t op1_size;
				bool op1_is_reg;
				uint32_t op1_reg;

			};
		}
	}
}

#endif /* X86DECODER_H */

