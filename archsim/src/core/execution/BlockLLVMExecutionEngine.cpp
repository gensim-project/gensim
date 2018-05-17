/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "blockjit/block-compiler/transforms/Transform.h"
#include "core/execution/BlockLLVMExecutionEngine.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "core/MemoryInterface.h"
#include "system.h"
#include "translate/jit_funs.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/JITSymbol.h>

using namespace archsim;
using namespace archsim::core::execution;

static llvm::TargetMachine *GetNativeMachine() {
	llvm::TargetMachine *machine = nullptr;
	
	if(machine == nullptr) {
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		machine = llvm::EngineBuilder().selectTarget();
		machine->setOptLevel(llvm::CodeGenOpt::None);
		machine->setFastISel(true);
	}
	return machine;
}

BlockLLVMExecutionEngine::BlockLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator) : 
	BlockJITExecutionEngine(translator),
	target_machine_(GetNativeMachine()),
	linker_([]() { return std::make_shared<llvm::SectionMemoryManager>(); }),
	compiler_(linker_, llvm::orc::SimpleCompiler(*target_machine_))
{
	
}

bool BlockLLVMExecutionEngine::buildBlockJITIR(thread::ThreadInstance* thread, archsim::Address block_pc, captive::arch::jit::TranslationContext& ctx)
{
	auto translator = GetTranslator();
	translator->InitialiseFeatures(thread);
	translator->InitialiseIsaMode(thread);
	translator->SetDecodeContext(thread->GetEmulationModel().GetNewDecodeContext(*thread));
	
	captive::shared::IRBuilder builder;
	builder.SetContext(&ctx);
	builder.SetBlock(ctx.alloc_block());
	
	if(!translator->build_block(thread, block_pc, builder)) {
		return false;
	}
	
	// optimise ir
	captive::arch::jit::transforms::SortIRTransform().Apply(ctx);
	captive::arch::jit::transforms::ReorderBlocksTransform().Apply(ctx);
	captive::arch::jit::transforms::ThreadJumpsTransform().Apply(ctx);
	captive::arch::jit::transforms::DeadBlockEliminationTransform().Apply(ctx);
	captive::arch::jit::transforms::MergeBlocksTransform().Apply(ctx);
	captive::arch::jit::transforms::ConstantPropTransform().Apply(ctx);
	captive::arch::jit::transforms::SortIRTransform().Apply(ctx);
	captive::arch::jit::transforms::RegValueReuseTransform().Apply(ctx);
	captive::arch::jit::transforms::RegStoreEliminationTransform().Apply(ctx);
	captive::arch::jit::transforms::SortIRTransform().Apply(ctx);
	
	return true;
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
	
	std::unique_ptr<llvm::Module> module (new llvm::Module("mod_"+ std::to_string(block_pc.Get()), llvm_ctx_));
	module->setDataLayout(target_machine_->createDataLayout());
	
	captive::arch::jit::TranslationContext txln_ctx;
	buildBlockJITIR(thread, block_pc, txln_ctx);
	
	archsim::translate::adapt::BlockJITToLLVMAdaptor adaptor (llvm_ctx_);
	std::string fn_name = "fn_" + std::to_string(block_pc.Get());
	auto function = adaptor.AdaptIR(thread, module.get(), fn_name, txln_ctx);
	if(function == nullptr) {
		return false;
	}
	
	std::map<std::string, void *> jit_symbols;
	jit_symbols["cpuTakeException"] = (void*)cpuTakeException;
	jit_symbols["genc_adc_flags"] = (void*)genc_adc_flags;
	jit_symbols["blkRead8"] = (void*)blkRead8;
	jit_symbols["blkRead16"] = (void*)blkRead16;
	jit_symbols["blkRead32"] = (void*)blkRead32;
	jit_symbols["blkWrite8"] = (void*)cpuWrite8;
	jit_symbols["blkWrite16"] = (void*)cpuWrite16;
	jit_symbols["blkWrite32"] = (void*)cpuWrite32;
	
	auto resolver = llvm::orc::createLambdaResolver(
		[&](const std::string &name){
			if(auto sym = compiler_.findSymbol(name, false)) { return sym; } else { return llvm::JITSymbol(nullptr); }
		},
		[jit_symbols](const std::string &name) {
			if(jit_symbols.count(name)) { return llvm::JITSymbol((intptr_t)jit_symbols.at(name), llvm::JITSymbolFlags::Exported); }
			else return llvm::JITSymbol(nullptr);
		}	
	);
	
	auto handle = compiler_.addModule(std::move(module), std::move(resolver));
	
	if(!handle) {
		throw std::logic_error("Failed to JIT block");
	}
	
	auto symbol = compiler_.findSymbolIn(handle.get(), fn_name, true);
	auto address = symbol.getAddress();
	
	if(address) {
		block_txln_fn fn = (block_txln_fn)address.get();
		txln.SetFn(fn);
		
		phys_block_profile_.Insert(physaddr, txln);
		virt_block_cache_.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
		
		return true;
	} else {
		// if we failed to produce a translation, then try and stop the simulation
//		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
		return false;
	}
	
	return false;
}
