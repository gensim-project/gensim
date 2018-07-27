/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "util/ComponentManager.h"
#include "gensim/gensim_decode_context.h"

class Aarch64DecodeTranslationContext : public gensim::DecodeTranslateContext
{
	void Translate(archsim::core::thread::ThreadInstance *cpu, const gensim::BaseDecode &insn, gensim::DecodeContext &decode, captive::shared::IRBuilder &builder) override
	{
		// nothing necessary here
	}

};

//RegisterComponent(gensim::DecodeContext, RiscVDecodeContext, "riscv", "risc v", archsim::core::thread::ThreadInstance*);
RegisterComponent(gensim::DecodeTranslateContext, Aarch64DecodeTranslationContext, "aarch64", "ARMv8");
