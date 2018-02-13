/*
 * File:   gensim_decode.h
 * Author: s0803652
 *
 * Created on 20 October 2011, 17:26
 */

#ifndef _GENSIM_DISASM_H
#define _GENSIM_DISASM_H

#include <stdint.h>

#include <string>

namespace gensim
{

	class BaseDecode;

	class BaseDisasm
	{
	public:
		virtual std::string DisasmInstr(const gensim::BaseDecode &decode, uint32_t pc) = 0;
		virtual std::string GetInstrName(uint32_t instr_code) = 0;
	};
}

#endif /* _GENSIM_DISASM_H */
