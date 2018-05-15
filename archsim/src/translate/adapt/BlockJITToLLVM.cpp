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

#include "util/SimOptions.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Transforms/Scalar.h"

#include <sstream>
#include <fstream>

using namespace archsim::translate::adapt;
using namespace captive::arch::jit;
using namespace captive::shared;

BlockJITToLLVMAdaptor::BlockJITToLLVMAdaptor(llvm::LLVMContext& ctx) : ctx_(ctx) {

}


llvm::FunctionType *archsim::translate::adapt::GetBlockFunctionType(::llvm::LLVMContext &ctx) {
	// takes two void ptr types
	
	llvm::Type *voidty = ::llvm::Type::getInt8PtrTy(ctx, 0);
	
	llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {voidty, voidty}, false);
	return ftype;
}

llvm::Function* BlockJITToLLVMAdaptor::AdaptIR(archsim::core::thread::ThreadInstance *thread, llvm::Module* target_module, const std::string& name, const captive::arch::jit::TranslationContext &ctx) {
	llvm::Function *fun = (llvm::Function*)target_module->getOrInsertFunction(name, GetBlockFunctionType(ctx_));
	
	// add an entry block to the function
	auto entry_block = llvm::BasicBlock::Create(target_module->getContext(), "entry_block", fun);
	
	BlockJITLoweringContext lowerer (thread, target_module, fun);
	lowerer.Prepare(ctx);
	lowerer.PrepareLowerers(ctx);
	if(!lowerer.Lower(ctx)) {
		return nullptr;
	}
	
	// TODO: jump from entry block to first instruction block
	llvm::BranchInst::Create(lowerer.GetLLVMBlock(0), entry_block);
		
	{
		llvm::legacy::PassManager pm;
//		pm.add(new llvm::DataLayout(engine_->getDataLayout()));
		pm.add(llvm::createBasicAAWrapperPass());
		pm.add(llvm::createPromoteMemoryToRegisterPass());
		pm.add(llvm::createInstructionCombiningPass(false));
		pm.add(llvm::createReassociatePass());

		pm.run(*target_module);
	}
	
	if(archsim::options::Debug) {
		std::stringstream filename;
		filename << "llvm-" << std::hex << name << ".opt.ll";

		std::ofstream os_stream(filename.str().c_str(), std::ios_base::out);
		::llvm::raw_os_ostream str(os_stream);
		
		llvm::legacy::PassManager mpm;
		mpm.add(llvm::createPrintModulePass(str));
		
		mpm.run(*target_module);
	}
	
	return fun;
}
