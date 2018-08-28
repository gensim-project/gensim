/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/execution/BlockLLVMExecutionEngine.h"
#include "core/execution/BlockToLLVMExecutionEngine.h"
#include "core/execution/ExecutionEngineFactory.h"
#include "core/thread/ThreadInstance.h"
#include "core/MemoryInterface.h"
#include "system.h"
#include "translate/jit_funs.h"
#include "gensim/gensim_processor_api.h"
#include "util/LogContext.h"
#include "gensim/gensim_translate.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

using namespace archsim::core::execution;

UseLogContext(LogBlockJitCpu);

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

BlockLLVMExecutionEngine::BlockLLVMExecutionEngine(gensim::BaseLLVMTranslate *translator) :
	target_machine_(GetNativeMachine()),
	memory_manager_(std::make_shared<BlockJITLLVMMemoryManager>(GetMemAllocator())),
	linker_([&]()
{
	return memory_manager_;
}),
compiler_(linker_, llvm::orc::SimpleCompiler(*target_machine_)),
translator_(translator)
{

}


ExecutionEngineThreadContext* BlockLLVMExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

bool BlockLLVMExecutionEngine::translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling)
{
	checkCodeSize();

	captive::shared::block_txln_fn fn;
	if(lookupBlock(thread, block_pc, fn)) {
		return true;
	}

	// Translate WITH side effects. If a fault occurs, stop translating this block
	Address physaddr (0);
	archsim::TranslationResult fault = thread->GetFetchMI().PerformTranslation(block_pc, physaddr, false, true, true);

	archsim::blockjit::BlockTranslation txln;
	// we couldn't find the block in the physical profile, so create a new translation
	thread->GetEmulationModel().GetSystem().GetCodeRegions().MarkRegionAsCode(PhysicalAddress(physaddr.PageBase().Get()));

	std::unique_ptr<llvm::Module> module (new llvm::Module("mod_"+ std::to_string(block_pc.Get()), llvm_ctx_));
	module->setDataLayout(target_machine_->createDataLayout());

	std::string fn_name = "fn_" + std::to_string(block_pc.Get());
	auto function = translateToFunction(thread, physaddr, fn_name, module);

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

}

llvm::Function* BlockLLVMExecutionEngine::translateToFunction(archsim::core::thread::ThreadInstance* thread, Address phys_pc, const std::string fn_name, std::unique_ptr<llvm::Module>& llvm_module)
{
	llvm::Module *module = llvm_module.get();
	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction(fn_name, getFunctionType());

	auto &isa = thread->GetArch().GetISA(thread->GetModeID());
	auto jumpinfo = isa.GetNewJumpInfo();
	auto decode = isa.GetNewDecode();

	auto entry_block = llvm::BasicBlock::Create(llvm_ctx_, "", fn);
	llvm::IRBuilder<> builder (entry_block);

	while(true) {
		isa.DecodeInstr(phys_pc, &thread->GetFetchMI(), *decode);

		// translate instruction
		translator_->TranslateInstruction(thread, decode, phys_pc, fn, (void*)&builder);

		// check for block exit
		gensim::JumpInfo ji;
		jumpinfo->GetJumpInfo(decode, phys_pc, ji);

		if(ji.IsJump) {
			break;
		}
		phys_pc += decode->Instr_Length;
	}

	delete jumpinfo;
	delete decode;

	return fn;
}

llvm::FunctionType *BlockLLVMExecutionEngine::getFunctionType()
{
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(llvm_ctx_);

	return llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(llvm_ctx_), {i8ptrty, i8ptrty}, false);
}

class NullLLVMTranslator : public gensim::BaseLLVMTranslate
{
public:
	virtual ~NullLLVMTranslator()
	{

	}

	bool TranslateInstruction(archsim::core::thread::ThreadInstance* thread, gensim::BaseDecode* decode, archsim::Address phys_pc, llvm::Function* fn, void* irbuilder) override
	{
		UNIMPLEMENTED;
	}
};

ExecutionEngine* BlockLLVMExecutionEngine::Factory(const archsim::module::ModuleInfo* module, const std::string& cpu_prefix)
{
	std::string entry_name = cpu_prefix + "LLVMTranslator";
	if(!module->HasEntry(entry_name)) {
		LC_ERROR(LogBlockJitCpu) << "Could not find LLVMTranslator in target module";
		return nullptr;
	}

	auto translator_entry = module->GetEntry<archsim::module::ModuleLLVMTranslatorEntry>(entry_name)->Get();
	return new BlockLLVMExecutionEngine(translator_entry);
}

static archsim::core::execution::ExecutionEngineFactoryRegistration registration("LLVMBlockJIT", 80, archsim::core::execution::BlockLLVMExecutionEngine::Factory);
