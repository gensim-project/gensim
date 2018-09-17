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

	std::unique_ptr<llvm::Module> module (new llvm::Module("mod_"+ std::to_string(block_pc.Get()), llvm_ctx_));
	module->setDataLayout(target_machine_->createDataLayout());

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

	std::map<std::string, void *> jit_symbols;

	jit_symbols["cpuTrap"] = (void*)cpuTrap;
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
	jit_symbols["cpuWrite8"] = (void*)cpuWrite8;
	jit_symbols["cpuWrite16"] = (void*)cpuWrite16;
	jit_symbols["cpuWrite32"] = (void*)cpuWrite32;
	jit_symbols["cpuWrite64"] = (void*)cpuWrite64;

	jit_symbols["cpuTraceOnlyMemWrite8"] = (void*)cpuTraceOnlyMemWrite8;
	jit_symbols["cpuTraceOnlyMemWrite16"] = (void*)cpuTraceOnlyMemWrite16;
	jit_symbols["cpuTraceOnlyMemWrite32"] = (void*)cpuTraceOnlyMemWrite32;
	jit_symbols["cpuTraceOnlyMemWrite64"] = (void*)cpuTraceOnlyMemWrite64;

	jit_symbols["cpuTraceOnlyMemRead8"] = (void*)cpuTraceOnlyMemRead8;
	jit_symbols["cpuTraceOnlyMemRead16"] = (void*)cpuTraceOnlyMemRead16;
	jit_symbols["cpuTraceOnlyMemRead32"] = (void*)cpuTraceOnlyMemRead32;
	jit_symbols["cpuTraceOnlyMemRead64"] = (void*)cpuTraceOnlyMemRead64;

	jit_symbols["cpuTraceRegBankWrite"] = (void*)cpuTraceRegBankWrite;
	jit_symbols["cpuTraceRegWrite"] = (void*)cpuTraceRegWrite;
	jit_symbols["cpuTraceRegBankRead"] = (void*)cpuTraceRegBankRead;
	jit_symbols["cpuTraceRegRead"] = (void*)cpuTraceRegRead;

	jit_symbols["cpuTraceInstruction"] = (void*)cpuTraceInstruction;
	jit_symbols["cpuTraceInsnEnd"] = (void*)cpuTraceInsnEnd;

	jit_symbols["__umodti3"] = (void*)uremi128;
	jit_symbols["__udivti3"] = (void*)udivi128;
	jit_symbols["__modti3"] = (void*)remi128;
	jit_symbols["__divti3"] = (void*)divi128;

	// todo: change these to actual bitcode
	jit_symbols["txln_shunt___builtin_f32_is_snan"] = (void*)shunt_builtin_f32_is_snan;
	jit_symbols["txln_shunt___builtin_f32_is_qnan"] = (void*)shunt_builtin_f32_is_qnan;
	jit_symbols["txln_shunt___builtin_f64_is_snan"] = (void*)shunt_builtin_f64_is_snan;
	jit_symbols["txln_shunt___builtin_f64_is_qnan"] = (void*)shunt_builtin_f64_is_qnan;

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

	auto pc_desc = thread->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");

	archsim::translate::tx_llvm::LLVMTranslationContext ctx(llvm_ctx_, fn, thread);

	while(true) {
		auto result = isa.DecodeInstr(phys_pc, &thread->GetFetchMI(), *decode);
		if(result) {
			// failed to decode instruction
			LC_ERROR(LogBlockJitCpu) << "Failed to decode instruction at pc " << phys_pc;
			return nullptr;
		}

		if(archsim::options::Trace) {
			builder.CreateCall(ctx.Functions.cpuTraceInstruction, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i64, phys_pc.Get()), llvm::ConstantInt::get(ctx.Types.i32, decode->ir), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 1)});
		}

		gensim::JumpInfo ji;
		jumpinfo->GetJumpInfo(decode, phys_pc, ji);

		// if we're about to execute a jump, update the PC first
		if(ji.IsJump) {
			// todo: fix this for 32 bit & full system
			translator_->EmitRegisterWrite(builder, ctx, pc_desc.GetEntrySize(), pc_desc.GetOffset(), llvm::ConstantInt::get(ctx.Types.i64, phys_pc.Get()));
		}

		// translate instruction

		if(!translator_->TranslateInstruction(builder, ctx, thread, decode, phys_pc, fn)) {
			LC_ERROR(LogBlockJitCpu) << "Failed to translate instruction at pc " << phys_pc;
			return nullptr;
		}

		if(archsim::options::Trace) {
			builder.CreateCall(ctx.Functions.cpuTraceInsnEnd, {ctx.GetThreadPtr(builder)});
		}

		// check for block exit
		if(ji.IsJump) {
			break;
		}
		phys_pc += decode->Instr_Length;
	}

	builder.CreateRet(llvm::ConstantInt::get(ctx.Types.i32, 0));

	delete jumpinfo;
	delete decode;

	return fn;
}

llvm::FunctionType *BlockLLVMExecutionEngine::getFunctionType()
{
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(llvm_ctx_);

	return llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(llvm_ctx_), {i8ptrty, i8ptrty}, false);
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
