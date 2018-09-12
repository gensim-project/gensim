/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/AsynchronousTranslationWorker.h"
#include "translate/AsynchronousTranslationManager.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMTranslation.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "blockjit/BlockJitTranslate.h"
#include "gensim/gensim_processor_api.h"

#include "util/LogContext.h"
#include "translate/profile/Block.h"

#include "translate/Translation.h"
#include "translate/jit_funs.h"
#include "translate/llvm/LLVMMemoryManager.h"

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_os_ostream.h>

#include <iostream>
#include <iomanip>
#include <set>

UseLogContext(LogTranslate);
UseLogContext(LogWorkQueue);

using namespace archsim::translate;
using namespace archsim::translate::translate_llvm;

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
		machine->setFastISel(true);
	}

	lock.unlock();
	return machine;
}

AsynchronousTranslationWorker::AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::BaseLLVMTranslate *translate) :
	Thread("Txln Worker"),
	mgr(mgr),
	terminate(false),
	id(id),
	optimiser(NULL),
	translate_(translate),
	target_machine_(GetNativeMachine()),
	memory_manager_(new archsim::translate::translate_llvm::LLVMMemoryManager(code_pool, code_pool)),
	linker_([&]()
{
	return memory_manager_;
}),
compiler_(linker_, llvm::orc::SimpleCompiler(*target_machine_))
{

}

/**
 * The main thread entry-point for this translation worker.
 */
void AsynchronousTranslationWorker::run()
{

	// Instantiate the unique lock for the work unit queue.
	std::unique_lock<std::mutex> queue_lock(mgr.work_unit_queue_lock, std::defer_lock);

	// Loop until told to terminate.
	while (!terminate) {
		// Acquire the work unit queue lock.
		queue_lock.lock();

		// Loop until work becomes available, or we're asked to terminate.
		while (mgr.work_unit_queue.empty() && !terminate) {
			// first do a bit of busy work
			code_pool.GC();

			// Wait on the work unit queue signal.
			mgr.work_unit_queue_cond.wait(queue_lock);
		}

		// If we woke up because we were asked to terminate, then release the
		// work unit queue lock and exit the loop.
		if (terminate) {
			queue_lock.unlock();
			break;
		}

		// Dequeue the translation work unit.
		TranslationWorkUnit *unit;

		// Skip over work units that are invalid.
		do {
			// Detect the case when we've cleared the work unit queue of valid translations.
			if (mgr.work_unit_queue.empty()) {
				unit = NULL;
				break;
			}

			unit = mgr.work_unit_queue.top();
			mgr.work_unit_queue.pop();

			if (!unit->GetRegion().IsValid()) {
				LC_DEBUG1(LogWorkQueue) << "[DEQUEUE] Skipping " << *unit;
				delete unit;

				//Setting unit to null here will cause us to break out of this do, while loop,
				//and then hit the continue a little later on in the main while loop. This
				//entire loop is somewhat too complicated and should be broken up.
				unit = NULL;
				break;
			}
		} while (!terminate && !unit->GetRegion().IsValid());

		// Release the queue lock.
		queue_lock.unlock();

		if (terminate || !unit)
			continue;

		LC_DEBUG1(LogWorkQueue) << "[DEQUEUE] Dequeueing " << *unit << ", queue length " << mgr.work_unit_queue.size() << ", @ " << (uint32_t)id;

		// Perform the translation, and destroy the translation work unit.
		::llvm::LLVMContext llvm_ctx;
		Translate(llvm_ctx, *unit);
		delete unit;
	}

}

/**
 * Stops the translation worker thread.
 */
void AsynchronousTranslationWorker::stop()
{
	// Set the termination flag.
	terminate = true;

	// Signal the thread to wake-up, if it's waiting on the work unit queue lock.
	mgr.work_unit_queue_lock.lock();
	mgr.work_unit_queue_cond.notify_all();
	mgr.work_unit_queue_lock.unlock();

	// Wait for the thread to terminate.
	join();
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
LLVMTranslation* AsynchronousTranslationWorker::CompileModule(TranslationWorkUnit& unit, ::llvm::Module* module, llvm::Function* function)
{
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

	std::string function_name = function->getName();

	auto handle = compiler_.addModule(std::unique_ptr<llvm::Module>(module), std::move(resolver));
	if(!handle) {
		return nullptr;
	}

	auto symbol = compiler_.findSymbolIn(handle.get(), function_name, true);

	auto address = symbol.getAddress();
	if(!address) {
		return nullptr;
	}


	LLVMTranslation *txln = new LLVMTranslation((LLVMTranslation::translation_fn)address.get(), nullptr);
	for(auto i : unit.GetBlocks()) {
		if(i.second->IsEntryBlock()) {
			txln->AddContainedBlock(i.first);
		}
	}

	if(archsim::options::Debug) {
		std::ofstream str("code_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()));
		str.write((char*)address.get(), memory_manager_->getAllocatedCodeSize((void*)address.get()));
	}

	return txln;
}

llvm::BasicBlock *BuildDispatchBlock(gensim::BaseLLVMTranslate *txlt, TranslationWorkUnit &unit, llvm::Function *function, std::map<archsim::Address, llvm::BasicBlock*> &blocks, llvm::BasicBlock *exit_block)
{
	std::set<archsim::Address> entry_blocks;
	auto *entry_block = &function->getEntryBlock();
	for(auto b : unit.GetBlocks()) {
		auto block_block = llvm::BasicBlock::Create(function->getContext(), "block_" + std::to_string(b.first.Get()), function);
		blocks[b.first] = block_block;

		if(b.second->IsEntryBlock()) {
			entry_blocks.insert(b.first);
		}
	}

	llvm::BasicBlock *dispatch_block = llvm::BasicBlock::Create(function->getContext(), "dispatch", function);
	llvm::BranchInst::Create(dispatch_block, entry_block);

	llvm::IRBuilder<> builder(dispatch_block);
	tx_llvm::LLVMTranslationContext ctx(function->getContext(), function->getParent(), unit.GetThread());
	ctx.SetBuilder(builder);

	auto pc = txlt->EmitRegisterRead(ctx, 8, 128);

	auto switch_inst = builder.CreateSwitch(pc, exit_block, blocks.size());
	for(auto block : blocks) {
		if(entry_blocks.count(block.first)) {
			switch_inst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(function->getContext()), unit.GetRegion().GetPhysicalBaseAddress().Get() + block.first.GetPageOffset()), block.second);
		}
	}

	return dispatch_block;
}

/**
 * Translates the given translation work unit.
 * @param llvm_ctx The LLVM context to perform the translation with.
 * @param unit The unit to translate.
 */
void AsynchronousTranslationWorker::Translate(::llvm::LLVMContext& llvm_ctx, TranslationWorkUnit& unit)
{
	// If debugging is turned on, dump the control-flow graph for this work unit.
	if (archsim::options::Debug) {
		unit.DumpGraph();
	}

	if (archsim::options::Verbose) {
		units.inc();
		blocks.inc(unit.GetBlockCount());
	}

	LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translating: " << unit;

	// Create a new LLVM translation context.
	LLVMOptimiser opt;

	// Create a new llvm module to contain the translation
	llvm::Module *module = new llvm::Module("region_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()), llvm_ctx);
	module->setDataLayout(GetNativeMachine()->createDataLayout());

	auto i8ptrty = llvm::Type::getInt8PtrTy(llvm_ctx);
	llvm::FunctionType *fn_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), {i8ptrty, i8ptrty}, false);
	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction("fn", fn_type);
	auto entry_block = llvm::BasicBlock::Create(llvm_ctx, "entry_block", fn);
	auto exit_block = llvm::BasicBlock::Create(llvm_ctx, "exit_block", fn);

	gensim::BaseLLVMTranslate *translate = translate_;
//	translate->InitialiseFeatures(unit.GetThread());
//	translate->InitialiseIsaMode(unit.GetThread());
//	translate->SetDecodeContext(unit.GetThread()->GetEmulationModel().GetNewDecodeContext(*unit.GetThread()));

	std::map<Address, llvm::BasicBlock*> block_map;

	auto ji = unit.GetThread()->GetArch().GetISA(0).GetNewJumpInfo();

	llvm::BasicBlock *dispatch_block = BuildDispatchBlock(translate, unit, fn, block_map, exit_block);
	tx_llvm::LLVMTranslationContext ctx(module->getContext(), module, unit.GetThread());

	for(auto block : unit.GetBlocks()) {
		llvm::BasicBlock *startblock = block_map.at(block.first);
		llvm::IRBuilder<> builder(startblock);
		ctx.SetBuilder(builder);
		ctx.SetBlock(startblock);

		if(archsim::options::Verbose) {
			// increment instruction counter
			translate->EmitIncrementCounter(ctx, unit.GetThread()->GetMetrics().InstructionCount, block.second->GetInstructions().size());
			translate->EmitIncrementCounter(ctx, unit.GetThread()->GetMetrics().JITInstructionCount, block.second->GetInstructions().size());
		}

		for(auto insn : block.second->GetInstructions()) {
			auto insn_pc = unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset();
			if(archsim::options::Trace) {
				builder.CreateCall(ctx.Functions.cpuTraceInstruction, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get()), llvm::ConstantInt::get(ctx.Types.i32, insn->GetDecode().ir), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 1)});
			}

			translate->TranslateInstruction(ctx, unit.GetThread(), &insn->GetDecode(), Address(unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset()), fn);
			ctx.ResetRegisters();

			gensim::JumpInfo jumpinfo;
			ji->GetJumpInfo(&insn->GetDecode(), Address(0), jumpinfo);
			if(!jumpinfo.IsJump) {
				translate_->EmitRegisterWrite(ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, (insn->GetOffset() + unit.GetRegion().GetPhysicalBaseAddress() + insn->GetDecode().Instr_Length).Get()));
			}

			if(archsim::options::Trace) {
				builder.CreateCall(ctx.Functions.cpuTraceInsnEnd, {ctx.GetThreadPtr()});
			}
		}

		auto pc_val = translate_->EmitRegisterRead(ctx, 8, 128);
		llvm::SwitchInst *out_switch = builder.CreateSwitch(pc_val, dispatch_block);
		for(auto &succ : block.second->GetSuccessors()) {
			auto succ_addr = (unit.GetRegion().GetPhysicalBaseAddress() + succ->GetOffset());

			if(block_map.count(succ->GetOffset())) {
				out_switch->addCase(llvm::ConstantInt::get(ctx.Types.i64, succ_addr.Get()), block_map.at(succ->GetOffset()));
			}
		}
	}

	llvm::ReturnInst::Create(llvm_ctx, nullptr, exit_block);

	if(archsim::options::Debug) {
		std::ofstream str("fn_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()));
		llvm::raw_os_ostream llvm_str (str);
		module->print(llvm_str, nullptr);
	}

	// optimise
	opt.Optimise(module, target_machine_->createDataLayout());

	// compile
	auto txln = CompileModule(unit, module, fn);

	if (!txln) {
		LC_ERROR(LogTranslate) << "[" << (uint32_t)id << "] Translation Failed: " << unit;
	} else {
		LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translation Succeeded: " << unit;

		if (archsim::options::Verbose) {
			txlns.inc();
		}

		if (!mgr.MarkTranslationAsComplete(unit.GetRegion(), *txln)) {
			delete txln;
		}
	}
}

void AsynchronousTranslationWorker::PrintStatistics(std::ostream& stream)
{
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.generation.GetElapsedS();
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.optimisation.GetElapsedS();
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.compilation.GetElapsedS();
	stream << std::endl;
}
