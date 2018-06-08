/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/thread/ThreadInstance.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "translate/TranslationContext.h"
#include "blockjit/block-compiler/lowering/InstructionLowerer.h"
#include "blockjit/IRPrinter.h"

#include "util/SimOptions.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar/GVN.h>

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
	
	BlockJITLoweringContext lowerer (thread->GetArch(), thread->GetStateBlock().GetDescriptor(), target_module, fun);
	lowerer.Prepare(ctx);
	lowerer.PrepareLowerers(ctx);
	if(!lowerer.Lower(ctx)) {
		throw std::logic_error("Failed to translate block");
	}
	
	// TODO: jump from entry block to first instruction block
	llvm::BranchInst::Create(lowerer.GetLLVMBlock(0), entry_block);
	

	if(archsim::options::Debug) {
		std::stringstream filename;
		filename << "llvm-" << std::hex << name << ".ll";

		std::ofstream os_stream(filename.str().c_str(), std::ios_base::out);
		::llvm::raw_os_ostream str(os_stream);
		
		llvm::legacy::PassManager mpm;
		mpm.add(llvm::createPrintModulePass(str));
		
		mpm.run(*target_module);
		
		filename.str("");
		filename << "blockjit-" << std::hex << name << ".blockjit";
		std::ofstream blockjit_file(filename.str(), std::ios_base::out);
		archsim::blockjit::IRPrinter printer;
		printer.DumpIR(blockjit_file, ctx);
	}	
	
	optimiser_.Optimise(target_module, target_module->getDataLayout());
	
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
