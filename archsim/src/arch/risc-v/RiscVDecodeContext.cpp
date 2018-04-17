/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/gensim_decode_context.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "gensim/gensim_processor.h"
#include "util/ComponentManager.h"

using namespace archsim::arch::riscv;

RiscVDecodeContext::RiscVDecodeContext(gensim::Processor *cpu) : DecodeContext(cpu)
{

}

uint32_t RiscVDecodeContext::DecodeSync(Address address, uint32_t mode, gensim::BaseDecode& target)
{
	return GetCPU()->DecodeInstr(address.Get(), mode, target);
}

class RiscVDecodeTranslationContext : public gensim::DecodeTranslateContext
{
	void Translate(gensim::BaseDecode& insn, gensim::DecodeContext& decode, captive::shared::IRBuilder &builder) override
	{
		// nothing necessary here
	}

};

RegisterComponent(gensim::DecodeContext, RiscVDecodeContext, "riscv", "risc v", gensim::Processor*);
RegisterComponent(gensim::DecodeTranslateContext, RiscVDecodeTranslationContext, "riscv", "risc v");