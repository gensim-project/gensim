/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

#include <llvm/IR/Function.h>

using namespace archsim::translate::adapt;


bool BlockJITCALLLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &target = insn->operands[0];
	
	std::vector<llvm::Value*> arg_values;
	std::vector<llvm::Type*> arg_types;
	
	auto thread_ptr = GetContext().GetThreadPtr();
	auto thread_ptr_type = thread_ptr->getType();
	
	arg_values.push_back(thread_ptr);
	arg_types.push_back(thread_ptr_type);
	
	for(int i = 1; i < insn->operands.size(); ++i) {
		if(insn->operands[i].type == IROperand::NONE) {
			break;
		}
		
		arg_values.push_back(GetValueFor(insn->operands[i]));
		arg_types.push_back(GetContext().GetLLVMType(insn->operands[i]));
	}
	
	llvm::Type *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(GetContext().GetLLVMContext()), arg_types, false)->getPointerTo(0);
	
	auto fn_value = GetBuilder().CreateIntToPtr(GetValueFor(target), ftype);
	GetBuilder().CreateCall(fn_value, arg_values);
	
	insn++;
	
	return true;
}
