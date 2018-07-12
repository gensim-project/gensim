/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_decode_context.h"
#include "arch/x86/X86DecodeContext.h"
#include "util/ComponentManager.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::arch::x86;

uint32_t X86DecodeContext::DecodeSync(archsim::MemoryInterface &interface, Address address, uint32_t mode, gensim::BaseDecode& target)
{
	return arch_.GetISA(mode).DecodeInstr(address, &interface, target);
}

class X86DecodeTranslationContext : public gensim::DecodeTranslateContext
{
	void Translate(archsim::core::thread::ThreadInstance *cpu, const gensim::BaseDecode &insn, gensim::DecodeContext &decode, captive::shared::IRBuilder &builder) override
	{
		// nothing necessary here
	}

};

RegisterComponent(gensim::DecodeTranslateContext, X86DecodeTranslationContext, "x86", "X86");
