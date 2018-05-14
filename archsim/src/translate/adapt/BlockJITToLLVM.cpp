/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITToLLVM.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "translate/TranslationContext.h"
#include "blockjit/block-compiler/lowering/InstructionLowerer.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

using namespace archsim::translate::adapt;
using namespace captive::arch::jit;
using namespace captive::shared;

BlockJITToLLVMAdaptor::BlockJITToLLVMAdaptor(llvm::LLVMContext& ctx) : ctx_(ctx) {

}


llvm::FunctionType *archsim::translate::adapt::GetBlockFunctionType(::llvm::LLVMContext &ctx) {
	llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {}, false);
	return ftype;
}

llvm::FunctionType *archsim::translate::adapt::GetIRBlockFunctionType(::llvm::LLVMContext &ctx) {
	llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {}, false);
	return ftype;
}

llvm::Function* BlockJITToLLVMAdaptor::AdaptIR(llvm::Module* target_module, const std::string& name, const captive::arch::jit::TranslationContext &ctx) {
	llvm::Function *fun = (llvm::Function*)target_module->getOrInsertFunction(name, GetBlockFunctionType(ctx_));
	
	BlockJITLoweringContext lowerer (target_module, fun);
	if(!lowerer.Lower(ctx)) {
		return nullptr;
	}
	
	return fun;
}
