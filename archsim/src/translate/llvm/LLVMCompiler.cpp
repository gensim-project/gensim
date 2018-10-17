/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_processor_api.h"
#include "translate/llvm/LLVMCompiler.h"
#include "translate/profile/Region.h"
#include "translate/jit_funs.h"

#include <llvm/Support/TargetSelect.h>

using namespace archsim::translate::translate_llvm;


static std::mutex perf_mutex_;
static FILE *perf_map_file_;

static void WritePerfMap(archsim::Address phys_addr, void *ptr, size_t size)
{
	perf_mutex_.lock();

	if(perf_map_file_ == nullptr) {
		std::string filename = "/tmp/perf-" + std::to_string(getpid()) + ".map";

		perf_map_file_ = fopen(filename.c_str(), "w");
	}


	fprintf(perf_map_file_, "%lx %x JIT-%x\n", ptr, size, phys_addr.Get());

	perf_mutex_.unlock();
}

static llvm::TargetMachine *GetNativeMachine()
{
	static std::mutex lock;
	lock.lock();

	llvm::TargetMachine *machine = nullptr;

	if(machine == nullptr) {
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		machine = llvm::EngineBuilder().selectTarget();
		machine->setOptLevel(llvm::CodeGenOpt::Aggressive);
		machine->setFastISel(false);
	}

	lock.unlock();
	return machine;
}

LLVMCompiler::LLVMCompiler() :
	target_machine_(GetNativeMachine()),
	memory_manager_(new archsim::translate::translate_llvm::LLVMMemoryManager(code_pool, code_pool)),
	linker_([&]()
{
	return memory_manager_;
}),
compiler_(linker_, llvm::orc::SimpleCompiler(*target_machine_))
{
	initJitSymbols();
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


void LLVMCompiler::initJitSymbols()
{
	jit_symbols_["memset"] = (void*)memset;
	jit_symbols_["cpuTrap"] = (void*)cpuTrap;
	jit_symbols_["cpuTakeException"] = (void*)cpuTakeException;
	jit_symbols_["cpuReadDevice"] = (void*)devReadDevice;
	jit_symbols_["cpuWriteDevice"] = (void*)devWriteDevice;
	jit_symbols_["cpuSetFeature"] = (void*)cpuSetFeature;

	jit_symbols_["cpuSetRoundingMode"] = (void*)cpuSetRoundingMode;
	jit_symbols_["cpuGetRoundingMode"] = (void*)cpuGetRoundingMode;
	jit_symbols_["cpuSetFlushMode"] = (void*)cpuSetFlushMode;
	jit_symbols_["cpuGetFlushMode"] = (void*)cpuGetFlushMode;

	jit_symbols_["genc_adc_flags"] = (void*)genc_adc_flags;
	jit_symbols_["blkRead8"] = (void*)blkRead8;
	jit_symbols_["blkRead16"] = (void*)blkRead16;
	jit_symbols_["blkRead32"] = (void*)blkRead32;
	jit_symbols_["blkRead64"] = (void*)blkRead64;
	jit_symbols_["cpuWrite8"] = (void*)cpuWrite8;
	jit_symbols_["cpuWrite16"] = (void*)cpuWrite16;
	jit_symbols_["cpuWrite32"] = (void*)cpuWrite32;
	jit_symbols_["cpuWrite64"] = (void*)cpuWrite64;

	jit_symbols_["cpuTraceOnlyMemWrite8"] = (void*)cpuTraceOnlyMemWrite8;
	jit_symbols_["cpuTraceOnlyMemWrite16"] = (void*)cpuTraceOnlyMemWrite16;
	jit_symbols_["cpuTraceOnlyMemWrite32"] = (void*)cpuTraceOnlyMemWrite32;
	jit_symbols_["cpuTraceOnlyMemWrite64"] = (void*)cpuTraceOnlyMemWrite64;

	jit_symbols_["cpuTraceOnlyMemRead8"] = (void*)cpuTraceOnlyMemRead8;
	jit_symbols_["cpuTraceOnlyMemRead16"] = (void*)cpuTraceOnlyMemRead16;
	jit_symbols_["cpuTraceOnlyMemRead32"] = (void*)cpuTraceOnlyMemRead32;
	jit_symbols_["cpuTraceOnlyMemRead64"] = (void*)cpuTraceOnlyMemRead64;

	jit_symbols_["cpuTraceRegBankWrite"] = (void*)cpuTraceRegBankWrite;
	jit_symbols_["cpuTraceRegWrite"] = (void*)cpuTraceRegWrite;
	jit_symbols_["cpuTraceRegBankRead"] = (void*)cpuTraceRegBankRead;
	jit_symbols_["cpuTraceRegRead"] = (void*)cpuTraceRegRead;

	jit_symbols_["cpuTraceInstruction"] = (void*)cpuTraceInstruction;
	jit_symbols_["cpuTraceInsnEnd"] = (void*)cpuTraceInsnEnd;

	jit_symbols_["__umodti3"] = (void*)uremi128;
	jit_symbols_["__udivti3"] = (void*)udivi128;
	jit_symbols_["__modti3"] = (void*)remi128;
	jit_symbols_["__divti3"] = (void*)divi128;

	// todo: change these to actual bitcode
	jit_symbols_["txln_shunt___builtin_f32_is_snan"] = (void*)shunt_builtin_f32_is_snan;
	jit_symbols_["txln_shunt___builtin_f32_is_qnan"] = (void*)shunt_builtin_f32_is_qnan;
	jit_symbols_["txln_shunt___builtin_f64_is_snan"] = (void*)shunt_builtin_f64_is_snan;
	jit_symbols_["txln_shunt___builtin_f64_is_qnan"] = (void*)shunt_builtin_f64_is_qnan;
}

LLVMCompiledModuleHandle LLVMCompiler::AddModule(llvm::Module* module)
{


	auto resolver = llvm::orc::createLambdaResolver(
	[this](const std::string &name) {
		// first, check our internal symbols
		if(jit_symbols_.count(name)) {
			return llvm::JITSymbol((intptr_t)jit_symbols_.at(name), llvm::JITSymbolFlags::Exported);
		}

		// then check more generally
		if(auto sym = compiler_.findSymbol(name, false)) {
			return sym;
		}

		// we couldn't find the symbol
		return llvm::JITSymbol(nullptr);
	},
	[this](const std::string &name) {
		if(jit_symbols_.count(name)) {
			return llvm::JITSymbol((intptr_t)jit_symbols_.at(name), llvm::JITSymbolFlags::Exported);
		} else return llvm::JITSymbol(nullptr);
	}
	                );

	auto handle = compiler_.addModule(std::unique_ptr<llvm::Module>(module), std::move(resolver));
	if(!handle) {
		throw std::logic_error("Failed to build module.");
	}
	return LLVMCompiledModuleHandle(handle.get());
}

LLVMTranslation * LLVMCompiler::GetTranslation(LLVMCompiledModuleHandle& handle, TranslationWorkUnit &twu)
{
	auto symbol = compiler_.findSymbolIn(handle.Get(), "fn", true);
	if(!symbol) {
		return nullptr;
	}

	auto address = symbol.getAddress();
	if(!address) {
		return nullptr;
	}

	LLVMTranslation *txln = new LLVMTranslation((LLVMTranslation::translation_fn)address.get(), nullptr);

	for(auto i : twu.GetBlocks()) {
		if(i.second->IsEntryBlock()) {
			txln->AddContainedBlock(i.first);
		}
	}

	if(archsim::options::Debug) {
		std::stringstream filename_str;
		filename_str << "code_" << std::hex << twu.GetRegion().GetPhysicalBaseAddress();
		std::ofstream str(filename_str.str().c_str());
		str.write((char*)address.get(), memory_manager_->getAllocatedCodeSize((void*)address.get()));
	}

	if(archsim::options::EnablePerfMap) {
		WritePerfMap(twu.GetRegion().GetPhysicalBaseAddress(), (void*)address.get(),  memory_manager_->getAllocatedCodeSize((void*)address.get()));
	}

	return txln;
}

