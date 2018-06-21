/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITSTDEVLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &dev = insn->operands[0];
	const auto &reg = insn->operands[1];
	const auto &val = insn->operands[2];

	auto thread_ptr = GetThreadPtr();
	auto dev_val = GetValueFor(dev);
	auto reg_val = GetValueFor(reg);
	auto val_val = GetValueFor(val);

	GetBuilder().CreateCall(GetContext().GetValues().cpuWriteDevicePtr, {thread_ptr, dev_val, reg_val, val_val});

	insn++;

	return true;
}

bool BlockJITLDDEVLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &dev = insn->operands[0];
	const auto &reg = insn->operands[1];
	const auto &val = insn->operands[2];

	auto thread_ptr = GetThreadPtr();
	auto dev_val = GetValueFor(dev);
	auto reg_val = GetValueFor(reg);
	auto target_val = GetBuilder().CreateAlloca(llvm::Type::getInt32Ty(GetContext().GetLLVMContext()), 0, nullptr);

	GetBuilder().CreateCall(GetContext().GetValues().cpuReadDevicePtr, {thread_ptr, dev_val, reg_val, target_val});
	auto result_val = GetBuilder().CreateLoad(target_val);

	SetValueFor(val, result_val);

	insn++;

	return true;
}
