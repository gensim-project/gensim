/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/gensim_decode_context.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "util/ComponentManager.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::arch::riscv;

RiscVDecodeContext::RiscVDecodeContext(archsim::core::thread::ThreadInstance *cpu) : DecodeContext(cpu)
{

}

uint32_t RiscVDecodeContext::DecodeSync(Address address, uint32_t mode, gensim::BaseDecode& target)
{
	return GetCPU()->GetArch().GetISA(mode).DecodeInstr(address, &GetCPU()->GetFetchMI(), target);
}

class RiscVDecodeTranslationContext : public gensim::DecodeTranslateContext
{
	void Translate(gensim::BaseDecode& insn, gensim::DecodeContext& decode, captive::shared::IRBuilder &builder) override
	{
		// nothing necessary here
	}

};

RegisterComponent(gensim::DecodeContext, RiscVDecodeContext, "riscv", "risc v", archsim::core::thread::ThreadInstance*);
RegisterComponent(gensim::DecodeTranslateContext, RiscVDecodeTranslationContext, "riscv", "risc v");