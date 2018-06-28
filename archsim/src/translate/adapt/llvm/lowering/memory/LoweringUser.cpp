/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITLDMEMUserLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &interface = insn->operands[0];
	const auto &offset = insn->operands[1];
	const auto &disp = insn->operands[2];
	const auto &dest = insn->operands[3];

	// Address space 256 corresponds to x86 GS segment
	llvm::PointerType *ptr_type = nullptr;
	switch(dest.size) {
		case 1:
			ptr_type = llvm::Type::getInt8PtrTy(GetContext().GetLLVMContext(), 256);
			break;
		case 2:
			ptr_type = llvm::Type::getInt16PtrTy(GetContext().GetLLVMContext(), 256);
			break;
		case 4:
			ptr_type = llvm::Type::getInt32PtrTy(GetContext().GetLLVMContext(), 256);
			break;
		case 8:
		default:
			UNIMPLEMENTED;
	}

	auto address_value = GetBuilder().CreateAdd(GetValueFor(offset), GetValueFor(disp));
	auto ptr = GetBuilder().CreateIntToPtr(address_value, ptr_type);
	auto data = GetBuilder().CreateLoad(ptr);

	SetValueFor(dest, data);

	insn++;

	return true;
}

bool BlockJITSTMEMUserLowering::Lower(const captive::shared::IRInstruction*& insn)
{
	const auto &interface = insn->operands[0];
	const auto &value = insn->operands[1];
	const auto &disp = insn->operands[2];
	const auto &offset = insn->operands[3];

	// Address space 256 corresponds to x86 GS segment
	llvm::PointerType *ptr_type = nullptr;
	switch(value.size) {
		case 1:
			ptr_type = llvm::Type::getInt8PtrTy(GetContext().GetLLVMContext(), 256);
			break;
		case 2:
			ptr_type = llvm::Type::getInt16PtrTy(GetContext().GetLLVMContext(), 256);
			break;
		case 4:
			ptr_type = llvm::Type::getInt32PtrTy(GetContext().GetLLVMContext(), 256);
			break;
		case 8:
		default:
			UNIMPLEMENTED;
	}

	auto address_value = GetBuilder().CreateAdd(GetValueFor(offset), GetValueFor(disp));
	auto ptr = GetBuilder().CreateIntToPtr(address_value, ptr_type);
	auto value_value = GetValueFor(value);

	GetBuilder().CreateStore(value_value, ptr);

	insn++;

	return true;
}
