/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/x86/X86JumpInfo.h"
#include "arch/x86/X86Decoder.h"

using namespace archsim::arch::x86;

X86JumpInfoProvider::~X86JumpInfoProvider()
{

}

void X86JumpInfoProvider::GetJumpInfo(const gensim::BaseDecode* instr, archsim::Address pc, gensim::JumpInfo& info)
{
	X86Decoder *d = (X86Decoder*)instr;
	info.IsJump = false;
	info.IsIndirect = false;
	info.IsConditional = false;

	// handle rep instructions ('indirect' jump for now)
	switch(d->Instr_Code) {
		case INST_x86_rep_movsb:
		case INST_x86_rep_movsd:
		case INST_x86_rep_movsq:
		case INST_x86_rep_stosb:
		case INST_x86_rep_stosd:
		case INST_x86_rep_stosq:
		case INST_x86_repe_cmpsb:
		case INST_x86_repne_scasb:
			info.IsJump = true;
			info.IsIndirect = false;
			info.IsConditional = true;
			info.JumpTarget = pc;
			break;
	}

	switch(d->Instr_Code) {
		case INST_x86_jmp:
		case INST_x86_jcond:
		case INST_x86_call:
		case INST_x86_ret:
			info.IsJump = true;
			break;
	}

	if(info.IsJump) {
		// relative if op0 is a relbr
		if(d->op0.is_relbr) {
			info.IsIndirect = false;
			info.JumpTarget = pc + d->op0.imm.value;
		} else {
			info.IsIndirect = true;
		}

		// only jcond is conditional
		if(d->Instr_Code == INST_x86_jcond) {
			info.IsConditional = true;
		}
	}
}
