/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/transforms/Transform.h"
#include "core/execution/BlockToLLVMExecutionEngine.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "core/MemoryInterface.h"
#include "system.h"
#include "translate/jit_funs.h"
#include "gensim/gensim_processor_api.h"
#include "core/execution/ExecutionEngineFactory.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/JITSymbol.h>

using namespace archsim;
using namespace archsim::core::execution;

BlockJITLLVMMemoryManager::BlockJITLLVMMemoryManager(wulib::MemAllocator& allocator) : allocator_(allocator)
{

}

uint8_t* BlockJITLLVMMemoryManager::allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName)
{
	auto section = (uint8_t*)allocator_.Allocate(Size);

	outstanding_code_sections_.push_back({section, Size});
	section_sizes_[section] = Size;

	return section;
}

uint8_t* BlockJITLLVMMemoryManager::allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName, bool IsReadOnly)
{
	return (uint8_t*)allocator_.Allocate(Size);
}

bool BlockJITLLVMMemoryManager::finalizeMemory(std::string* ErrMsg)
{
	for(auto i : outstanding_code_sections_) {
		__builtin___clear_cache(i.first, i.first + i.second);
	}
	outstanding_code_sections_.clear();

	return true;
}

static llvm::TargetMachine *GetNativeMachine()
{
	llvm::TargetMachine *machine = nullptr;

	if(machine == nullptr) {
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		machine = llvm::EngineBuilder().selectTarget();
		machine->setOptLevel(llvm::CodeGenOpt::Aggressive);
		machine->setFastISel(false);
		machine->setO0WantsFastISel(false);
	}
	return machine;
}

BlockToLLVMExecutionEngine::BlockToLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator) :
	BlockJITExecutionEngine(translator),
	target_machine_(GetNativeMachine()),
	memory_manager_(std::make_shared<BlockJITLLVMMemoryManager>(GetMemAllocator())),
	linker_([&]()
{
	return memory_manager_;
}),
compiler_(linker_, llvm::orc::SimpleCompiler(*target_machine_)),
adaptor_(llvm_ctx_)
{

}

bool BlockToLLVMExecutionEngine::buildBlockJITIR(thread::ThreadInstance* thread, archsim::Address block_pc, captive::arch::jit::TranslationContext& ctx, archsim::blockjit::BlockTranslation &txln)
{
	auto translator = GetTranslator();
	translator->InitialiseFeatures(thread);
	translator->InitialiseIsaMode(thread);
	auto decode_ctx = thread->GetEmulationModel().GetNewDecodeContext(*thread);
	translator->SetDecodeContext(decode_ctx);

	captive::shared::IRBuilder builder;
	builder.SetContext(&ctx);
	builder.SetBlock(ctx.alloc_block());

	if(!translator->build_block(thread, block_pc, builder)) {
		delete decode_ctx;
		return false;
	}

	translator->AttachFeaturesTo(txln);

	// optimise ir
//	captive::arch::jit::transforms::SortIRTransform().Apply(ctx);
//	captive::arch::jit::transforms::ReorderBlocksTransform().Apply(ctx);
//	captive::arch::jit::transforms::ThreadJumpsTransform().Apply(ctx);
//	captive::arch::jit::transforms::DeadBlockEliminationTransform().Apply(ctx);
//	captive::arch::jit::transforms::MergeBlocksTransform().Apply(ctx);
//	captive::arch::jit::transforms::ConstantPropTransform().Apply(ctx);
//	captive::arch::jit::transforms::SortIRTransform().Apply(ctx);
//	captive::arch::jit::transforms::RegValueReuseTransform().Apply(ctx);
//	captive::arch::jit::transforms::RegStoreEliminationTransform().Apply(ctx);
//	captive::arch::jit::transforms::SortIRTransform().Apply(ctx);

	delete decode_ctx;

	return true;
}


bool BlockToLLVMExecutionEngine::translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling)
{
	checkCodeSize();

	captive::shared::block_txln_fn fn;
	if(lookupBlock(thread, block_pc, fn)) {
		return true;
	}

	// Translate WITH side effects. If a fault occurs, stop translating this block
	Address physaddr (0);
	archsim::TranslationResult fault = thread->GetFetchMI().PerformTranslation(block_pc, physaddr, false, true, true);

	if(fault != TranslationResult::OK) {
		thread->TakeMemoryException(thread->GetFetchMI(), block_pc);
		return false;
	}

	// we couldn't find the block in the physical profile, so create a new translation
	thread->GetEmulationModel().GetSystem().GetCodeRegions().MarkRegionAsCode(PhysicalAddress(physaddr.PageBase().Get()));

	std::unique_ptr<llvm::Module> module (new llvm::Module("mod_"+ std::to_string(block_pc.Get()), llvm_ctx_));
	module->setDataLayout(target_machine_->createDataLayout());

	blockjit::BlockTranslation txln;
	captive::arch::jit::TranslationContext txln_ctx;
	if(!buildBlockJITIR(thread, block_pc, txln_ctx, txln)) {
		return false;
	}

	std::string fn_name = "fn_" + std::to_string(block_pc.Get());
	auto function = adaptor_.AdaptIR(thread, module.get(), fn_name, txln_ctx);
	if(function == nullptr) {
		return false;
	}

	std::map<std::string, void *> jit_symbols;

	jit_symbols["cpuTakeException"] = (void*)cpuTakeException;
	jit_symbols["cpuReadDevice"] = (void*)devReadDevice;
	jit_symbols["cpuWriteDevice"] = (void*)devWriteDevice;
	jit_symbols["cpuSetFeature"] = (void*)cpuSetFeature;

	jit_symbols["cpuSetRoundingMode"] = (void*)cpuSetRoundingMode;
	jit_symbols["cpuGetRoundingMode"] = (void*)cpuGetRoundingMode;
	jit_symbols["cpuSetFlushMode"] = (void*)cpuSetFlushMode;
	jit_symbols["cpuGetFlushMode"] = (void*)cpuGetFlushMode;

	jit_symbols["genc_adc_flags"] = (void*)genc_adc_flags;
	jit_symbols["blkRead8"] = (void*)blkRead8;
	jit_symbols["blkRead16"] = (void*)blkRead16;
	jit_symbols["blkRead32"] = (void*)blkRead32;
	jit_symbols["blkRead64"] = (void*)blkRead64;
	jit_symbols["blkWrite8"] = (void*)cpuWrite8;
	jit_symbols["blkWrite16"] = (void*)cpuWrite16;
	jit_symbols["blkWrite32"] = (void*)cpuWrite32;
	jit_symbols["blkWrite64"] = (void*)cpuWrite64;

	auto resolver = llvm::orc::createLambdaResolver(
	[&](const std::string &name) {
		// first, check our internal symbols
		if(jit_symbols.count(name)) {
			return llvm::JITSymbol((intptr_t)jit_symbols.at(name), llvm::JITSymbolFlags::Exported);
		}

		// then check more generally
		if(auto sym = compiler_.findSymbol(name, false)) {
			return sym;
		}

		// we couldn't find the symbol
		return llvm::JITSymbol(nullptr);
	},
	[jit_symbols](const std::string &name) {
		if(jit_symbols.count(name)) {
			return llvm::JITSymbol((intptr_t)jit_symbols.at(name), llvm::JITSymbolFlags::Exported);
		} else return llvm::JITSymbol(nullptr);
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
		txln.SetSize(memory_manager_->GetSectionSize((uint8_t*)fn));

		if(archsim::options::Debug) {
			txln.Dump("llvm-bin-" + std::to_string(physaddr.Get()));
		}

		registerTranslation(physaddr, block_pc, txln);

		return true;
	} else {
		// if we failed to produce a translation, then try and stop the simulation
//		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
		return false;
	}

	return false;
}

ExecutionEngine *BlockToLLVMExecutionEngine::Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix)
{
	std::string blockjit_entry_name = cpu_prefix + "BlockJITTranslator";
	std::string llvm_entry_name = cpu_prefix + "LLVMTranslator";
	if(module->HasEntry(blockjit_entry_name)) {
		auto translator_entry = module->GetEntry<archsim::module::ModuleBlockJITTranslatorEntry>(blockjit_entry_name)->Get();
		return new BlockToLLVMExecutionEngine(translator_entry);
	} else {
		return nullptr;
	}
}

static archsim::core::execution::ExecutionEngineFactoryRegistration registration("LLVMToBlockJIT", 90, archsim::core::execution::BlockToLLVMExecutionEngine::Factory);
