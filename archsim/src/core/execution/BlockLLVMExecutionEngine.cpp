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
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMBlockTranslator.h"
#include "translate/llvm/LLVMOptimiser.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/Support/raw_os_ostream.h>

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
		machine->setFastISel(true);
		machine->setO0WantsFastISel(false);
	}
	return machine;
}

BlockLLVMExecutionEngine::BlockLLVMExecutionEngine(gensim::BaseLLVMTranslate *translator) :
	BasicJITExecutionEngine(0),
	translator_(translator),
	compiler_(llvm_ctx_)
{

}


ExecutionEngineThreadContext* BlockLLVMExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

static unsigned __int128 uremi128(unsigned __int128 a, unsigned __int128 b)
{
	return a % b;
}
static unsigned __int128 udivi128(unsigned __int128 a, unsigned __int128 b)
{
	return a / b;
}
static __int128 remi128(__int128 a, __int128 b)
{
	return a % b;
}
static __int128 divi128(__int128 a, __int128 b)
{
	return a / b;
}

static bool shunt_builtin_f32_is_snan(archsim::core::thread::ThreadInstance* thread, float f)
{
	return thread->fn___builtin_f32_is_snan(f);
}
static bool shunt_builtin_f32_is_qnan(archsim::core::thread::ThreadInstance* thread, float f)
{
	return thread->fn___builtin_f32_is_qnan(f);
}
static bool shunt_builtin_f64_is_snan(archsim::core::thread::ThreadInstance* thread, double f)
{
	return thread->fn___builtin_f64_is_snan(f);
}
static bool shunt_builtin_f64_is_qnan(archsim::core::thread::ThreadInstance* thread, double f)
{
	return thread->fn___builtin_f64_is_qnan(f);
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

	std::unique_ptr<llvm::Module> module (new llvm::Module("mod_"+ std::to_string(block_pc.Get()), *llvm_ctx_.getContext()));

	std::string fn_name = "fn_" + std::to_string(block_pc.Get());
	auto function = translateToFunction(thread, physaddr, fn_name, module);

	if(function == nullptr) {
		return false;
	}

	if(archsim::options::Debug) {
		std::ofstream str(fn_name + ".ll");
		llvm::raw_os_ostream llvm_str(str);
		module->print(llvm_str, nullptr);

		llvm::legacy::FunctionPassManager fpm(module.get());
		fpm.add(llvm::createVerifierPass(true));
		fpm.doInitialization();
		fpm.run(*function);
	}

	archsim::translate::translate_llvm::LLVMOptimiser opt;
	opt.Optimise(module.get(), GetNativeMachine()->createDataLayout());

	if(archsim::options::Debug) {
		std::ofstream str(fn_name + "-opt.ll");
		llvm::raw_os_ostream llvm_str(str);
		module->print(llvm_str, nullptr);
	}

	auto handle = module.get();
	auto error = compiler_.AddModule(handle);


//	if(error) {
//		throw std::logic_error("Failed to JIT block");
//	}

	UNIMPLEMENTED;

//	auto symbol = compiler_.findSymbolIn(handle, fn_name, true);
//	auto address = symbol.getAddress();

//	if(address) {
//		block_txln_fn fn = (block_txln_fn)address.get();
//		txln.SetFn(fn);
//		txln.SetSize(memory_manager_->GetSectionSize((uint8_t*)fn));
//
//		if(archsim::options::Debug) {
//			txln.Dump("llvm-bin-" + std::to_string(physaddr.Get()));
//		}
//
//		registerTranslation(physaddr, block_pc, txln);
//
//		return true;
//	} else {
//		// if we failed to produce a translation, then try and stop the simulation
////		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
//		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
//		return false;
//	}

}

llvm::Function* BlockLLVMExecutionEngine::translateToFunction(archsim::core::thread::ThreadInstance* thread, Address phys_pc, const std::string fn_name, std::unique_ptr<llvm::Module>& llvm_module)
{
	llvm::Module *module = llvm_module.get();
	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction(fn_name, getFunctionType());

	auto &isa = thread->GetArch().GetISA(thread->GetModeID());
	auto jumpinfo = isa.GetNewJumpInfo();
	auto decode = isa.GetNewDecode();

	auto entry_block = llvm::BasicBlock::Create(GetContext(), "", fn);
	llvm::IRBuilder<> builder (entry_block);

	auto pc_desc = thread->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");

	// construct a translationblockunit
	archsim::translate::TranslationBlockUnit tbu (thread->GetPC(), 0, 1);

	Address insn_pc = thread->GetPC();
	Address block_start = insn_pc;
	while(true) {
		auto decode = isa.GetNewDecode();
		uint32_t result = isa.DecodeInstr(insn_pc, &thread->GetFetchMI(), *decode);

		if(result) {
			throw std::logic_error("Decode error");
		}

		tbu.AddInstruction(decode, insn_pc - tbu.GetOffset());

		gensim::JumpInfo ji;
		jumpinfo->GetJumpInfo(decode, insn_pc, ji);

		if(ji.IsJump) {
			break;
		}

		insn_pc += decode->Instr_Length;
	}

	// translate translationblockunit to llvm bitcode
	archsim::translate::translate_llvm::LLVMTranslationContext ctx (module->getContext(), fn, thread);
	archsim::translate::translate_llvm::LLVMBlockTranslator txltr(ctx, translator_, fn);

	auto end_block = txltr.TranslateBlock(block_start, tbu, entry_block);
	llvm::ReturnInst::Create(module->getContext(), llvm::ConstantInt::get(ctx.Types.i32, 0), end_block);

	ctx.Finalize();

	return fn;
}

llvm::FunctionType *BlockLLVMExecutionEngine::getFunctionType()
{
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(GetContext());

	return llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(GetContext()), {i8ptrty, i8ptrty}, false);
}
//
//class NullLLVMTranslator : public gensim::BaseLLVMTranslate
//{
//public:
//	virtual ~NullLLVMTranslator()
//	{
//
//	}
//
//	bool TranslateInstruction(archsim::core::thread::ThreadInstance* thread, gensim::BaseDecode* decode, archsim::Address phys_pc, llvm::Function* fn, void* irbuilder) override
//	{
//		UNIMPLEMENTED;
//	}
//};

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
