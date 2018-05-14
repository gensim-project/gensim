/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/BlockLLVMExecutionEngine.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "core/MemoryInterface.h"
#include "system.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>

using namespace archsim;
using namespace archsim::core::execution;

BlockLLVMExecutionEngine::BlockLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator) : BlockJITExecutionEngine(translator) {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
}


bool BlockLLVMExecutionEngine::translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) {
		// Look up the block in the cache, just in case we already have it translated
	captive::shared::block_txln_fn fn;
	if((fn = virt_block_cache_.Lookup(block_pc))) return true;

	// we missed in the block cache, so fall back to the physical profile
	Address physaddr (0);

	// Translate WITH side effects. If a fault occurs, stop translating this block
	TranslationResult fault = thread->GetFetchMI().PerformTranslation(block_pc, physaddr, false, true, true);

	if(fault != TranslationResult::OK) {
		thread->TakeMemoryException(thread->GetFetchMI(), block_pc);
		return false;
	}

	archsim::blockjit::BlockTranslation txln;
	if(phys_block_profile_.Get(physaddr, thread->GetFeatures(), txln)) {
		virt_block_cache_.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
		return true;
	}

	// we couldn't find the block in the physical profile, so create a new translation
	thread->GetEmulationModel().GetSystem().GetProfileManager().MarkRegionAsCode(PhysicalAddress(physaddr.PageBase().Get()));
	
	auto translator = GetTranslator();
	translator->InitialiseFeatures(thread);
	translator->InitialiseIsaMode(thread);
	translator->SetDecodeContext(thread->GetEmulationModel().GetNewDecodeContext(*thread));
	
	
	captive::arch::jit::TranslationContext txln_ctx;
	captive::shared::IRBuilder builder;
	builder.SetContext(&txln_ctx);
	builder.SetBlock(txln_ctx.alloc_block());
	
	llvm::Module *module = new llvm::Module("mod_"+ std::to_string(block_pc.Get()), llvm_ctx_);
	
	if(!translator->build_block(thread, block_pc, builder)) {
		return false;
	}
	
	archsim::translate::adapt::BlockJITToLLVMAdaptor adaptor (llvm_ctx_);
	auto function = adaptor.AdaptIR(module, "fn_" + std::to_string(block_pc.Get()), txln_ctx);
	if(function == nullptr) {
		return false;
	}
	
	::llvm::TargetOptions target_opts;
	target_opts.EnableFastISel = false;
	target_opts.PrintMachineCode = false;
	
	llvm::ExecutionEngine *engine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module))
		.setEngineKind(::llvm::EngineKind::JIT)
		.setOptLevel(::llvm::CodeGenOpt::Aggressive)
		.setRelocationModel(::llvm::Reloc::Static)
		.setCodeModel(::llvm::CodeModel::Large)
		.setTargetOptions(target_opts).create();
	
	assert(engine != nullptr);
	
	if(function) {
		block_txln_fn fn = (block_txln_fn)engine_->getPointerToFunction(function);
		txln.SetFn(fn);
		
		phys_block_profile_.Insert(physaddr, txln);
		virt_block_cache_.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
	} else {
		// if we failed to produce a translation, then try and stop the simulation
//		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
		return false;
	}
	
	return false;
}
