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
	INST_x86_mov
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
			};
		}
	}
}

#endif /* X86DECODER_H */

