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
		llvm::EngineBuilder builder;
		builder.setMAttrs<>(std::vector<std::string> {"+popcnt"});
		builder.setMCPU("haswell");
		machine = builder.selectTarget();
		machine->setOptLevel(llvm::CodeGenOpt::Aggressive);
		machine->setFastISel(false);
	}

	lock.unlock();
	return machine;
}

LLVMCompiler::LLVMCompiler(llvm::orc::ThreadSafeContext &ctx) :
	target_machine_(GetNativeMachine()),
	memory_manager_(new archsim::translate::translate_llvm::LLVMMemoryManager(code_pool, code_pool)),
	linker_(session_, [this]()
{
	return std::unique_ptr<llvm::RuntimeDyld::MemoryManager>(memory_manager_.get());
}),
ctx_(ctx)
{
	compiler_ = std::unique_ptr<CompileLayer>(new CompileLayer(session_, linker_, llvm::orc::SimpleCompiler(*target_machine_)));

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
	auto vmodule = session_.allocateVModule();
	auto &dylib = session_.createJITDylib(module->getName(), false);
	dylib.setGenerator([vmodule,this](llvm::orc::JITDylib &Parent, const llvm::orc::SymbolNameSet &Names) {
		llvm::orc::SymbolMap map;

		for(auto i : Names) {
			void *addr = jit_symbols_.at((*i).str());
			llvm::JITEvaluatedSymbol jes((uint64_t)addr, llvm::JITSymbolFlags(llvm::JITSymbolFlags::None));
			map[i] = jes;
		}

		llvm::orc::AbsoluteSymbolsMaterializationUnit *asmu = new llvm::orc::AbsoluteSymbolsMaterializationUnit(map, vmodule);
		llvm::Error e = Parent.define(std::unique_ptr<llvm::orc::MaterializationUnit>(asmu));
		if(e) {
			throw std::logic_error("Error defining symbols");
		}
		return Names;
	});

	auto error = compiler_->add(dylib, llvm::orc::ThreadSafeModule(std::unique_ptr<llvm::Module>(module), GetContext()), vmodule);

	if(error) {
		std::string str;
		llvm::raw_string_ostream stream(str);

		stream << error;

		throw std::logic_error("Failed to build module.: " + stream.str());
	}
	return LLVMCompiledModuleHandle(&dylib, vmodule);
}

LLVMTranslation * LLVMCompiler::GetTranslation(LLVMCompiledModuleHandle& handle, TranslationWorkUnit &twu)
{
	auto symbol = session_.lookup({handle.Lib}, llvm::StringRef("fn_" + std::to_string(twu.GetRegion().GetPhysicalBaseAddress().Get())));
	if(!symbol) {
		auto error (std::move(symbol.takeError()));
		return nullptr;
	}

	auto address = symbol->getAddress();
	if(!address) {
		return nullptr;
	}

	LLVMTranslation *txln = new LLVMTranslation((LLVMTranslation::translation_fn)address, nullptr);

	for(auto i : twu.GetBlocks()) {
		if(i.second->IsEntryBlock()) {
			txln->AddContainedBlock(i.first);
		}
	}

	if(archsim::options::Debug) {
		std::stringstream filename_str;
		filename_str << "code_" << std::hex << twu.GetRegion().GetPhysicalBaseAddress();
		std::ofstream str(filename_str.str().c_str());
		str.write((char*)address, memory_manager_->getAllocatedCodeSize((void*)address));
	}

	if(archsim::options::EnablePerfMap) {
		WritePerfMap(twu.GetRegion().GetPhysicalBaseAddress(), (void*)address,  memory_manager_->getAllocatedCodeSize((void*)address));
	}

	return txln;
}

