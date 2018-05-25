/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::translate::adapt;

bool BlockJITLDPCLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &target = insn->operands[0];

	assert(target.is_vreg());

	const auto *cpu = GetContext().GetThread();
	auto &PC = cpu->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");
	
	GetContext().SetValueFor(target, GetBuilder().CreateLoad(GetContext().GetRegisterPointer(PC, 0)));
	
	insn++;
	
	return true;
}
