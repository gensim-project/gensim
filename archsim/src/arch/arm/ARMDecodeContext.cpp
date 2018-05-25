/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/arm/ARMDecodeContext.h"
#include "blockjit/IRBuilder.h"
#include "gensim/gensim_decode.h"
#include "util/ComponentManager.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::arch::arm;

ARMDecodeContext::ARMDecodeContext(archsim::core::thread::ThreadInstance *cpu) : DecodeContext(cpu)
{
	auto itstate_desc = cpu->GetArch().GetRegisterFileDescriptor().GetEntries().at("ITSTATE");
	
	_itstate = (uint8_t*)cpu->GetRegisterFileInterface().GetEntry<uint8_t>("ITSTATE");
	_itstate_offset = itstate_desc.GetOffset();
}

uint32_t ARMDecodeContext::DecodeSync(Address address, uint32_t mode, gensim::BaseDecode& target)
{
	auto &cpu = *GetCPU();
	uint32_t fault = cpu.GetArch().GetISA(mode).DecodeInstr(address, &cpu.GetFetchMI(), target);
	if(fault) return fault;

	// TODO: have the IR for 16-bit instructions be filled into the lower 16 bits
	uint32_t ir = target.ir;
	if(target.Instr_Length == 2) ir >>= 16;

	target.ClearIsPredicated();

	if(*_itstate) {
		// Extract the cond and mask fields from ITSTATE
		uint8_t cond = *_itstate >> 5;
		uint8_t mask = *_itstate & 0x1f;

		// We need the whole ITSTATE field in case an exception occurs
		target.SetPredicateInfo(*_itstate);
		target.SetIsPredicated();

		// Update ITSTATE
		mask = (mask << 1) & 0x1f;
		if(mask == 0x10) {
			mask = 0;
			cond = 0;
		}

		// We need to update the real ITSTATE field as we go along in case
		// an instruction abort or interrupt occurs
		*_itstate = (cond << 5) | mask;
	} else {
		target.ClearIsPredicated();
	}

	// If we're in THUMB mode
	if(mode) {
		if((target.Instr_Length == 2) && ((ir & 0xff00) == 0xbf00)) {
			*_itstate = ir & 0xff;
		}
	} else {
		if((ir & 0xf0000000) < 0xe0000000) {
			target.SetIsPredicated();
			target.SetPredicateInfo(ir >> 28);
		}
	}

	return 0;
}

// When translating instructions, if the instruction is a thumb instruction,
// and it is predicated, write back the ITSTATE bits.
class ARMDecodeTranslateContext : public gensim::DecodeTranslateContext
{
public:
	void Translate(const gensim::BaseDecode &insn, gensim::DecodeContext &dcode, captive::shared::IRBuilder &builder) override
	{
		using namespace captive::shared;
		ARMDecodeContext &decode = (ARMDecodeContext&)dcode;

		if(insn.isa_mode) {
			if(insn.GetIsPredicated()) {
				// Write back modified ITSTATE
				uint8_t itstate = insn.GetPredicateInfo();
				uint8_t cond = itstate & 0xe0;
				uint8_t mask = itstate & 0x1f;
				mask = (mask << 1) & 0x1f;
				if(mask == 0x10) {
					mask = 0;
					cond = 0;
				}
				itstate = cond | mask;

				builder.streg(IROperand::const8(itstate), IROperand::const32(decode.GetITSTATEOffset()));
			}
		}
	}
};

RegisterComponent(gensim::DecodeTranslateContext, ARMDecodeTranslateContext, "armv7a", "arm decode translate context");
RegisterComponent(gensim::DecodeContext, ARMDecodeContext, "armv7a", "arm decode context", archsim::core::thread::ThreadInstance*);
