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

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>

#include <iostream>
#include <iomanip>

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
		machine->setOptLevel(llvm::CodeGenOpt::Default);
		machine->setFastISel(false);
	}

	lock.unlock();
	return machine;
}

AsynchronousTranslationWorker::AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::blockjit::BaseBlockJITTranslate *translate) :
	Thread("Txln Worker"),
	mgr(mgr),
	terminate(false),
	id(id),
	optimiser(NULL),
	translate_(translate),
	target_machine_(GetNativeMachine()),
	memory_manager_(new llvm::SectionMemoryManager()),
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

static llvm::Function *BuildDispatchFunction(llvm::Module *module, const TranslationWorkUnit &unit, const std::map<uint32_t, llvm::Function*> &fns)
{
	auto voidTy = llvm::Type::getVoidTy(module->getContext());
	auto i32Ty = llvm::Type::getInt32Ty(module->getContext());
	auto i32PtrTy = llvm::Type::getInt32PtrTy(module->getContext());
	auto i8PtrTy = llvm::Type::getInt8PtrTy(module->getContext());

	// TODO: check this if on 64 bit
	auto nativePtrTy = llvm::Type::getInt64Ty(module->getContext());

	llvm::FunctionType *ftype = llvm::FunctionType::get(voidTy, {i8PtrTy, i8PtrTy}, false);
	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction("region", ftype);

	auto arg_iterator = fn->arg_begin();
	auto arg_regptr = &*arg_iterator++;
	auto arg_stateptr = &*arg_iterator++;

	llvm::BasicBlock *entryblock = llvm::BasicBlock::Create(module->getContext(), "", fn);
	llvm::BasicBlock *switch_block = llvm::BasicBlock::Create(fn->getContext(), "", fn);
	llvm::BasicBlock *default_block = llvm::BasicBlock::Create(fn->getContext(), "", fn);
	llvm::IRBuilder<> builder(entryblock);

	// load PC
	// get PC descriptor
	const auto &pc_descriptor = unit.GetThread()->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");
	llvm::Value *pc_ptr = builder.CreatePtrToInt(arg_regptr, nativePtrTy);
	pc_ptr = builder.CreateAdd(pc_ptr, llvm::ConstantInt::get(nativePtrTy, pc_descriptor.GetOffset(), false));
	pc_ptr = builder.CreateIntToPtr(pc_ptr, i32PtrTy);
	llvm::Value *pc = builder.CreateLoad(pc_ptr);

	// check PC is on a virtual image
	auto page_base = builder.CreateAnd(pc, ~archsim::RegionArch::PageMask);
	llvm::Value *on_page = nullptr;
	if(unit.potential_virtual_bases.size() == 1) {
		on_page = builder.CreateICmpEQ(page_base, llvm::ConstantInt::get(i32Ty, unit.potential_virtual_bases.begin()->Get()));
	} else {
		UNIMPLEMENTED;
	}

	builder.CreateCondBr(on_page, switch_block, default_block);
	builder.SetInsertPoint(switch_block);

	// mask page offset
	auto page_offset = builder.CreateAnd(pc, archsim::RegionArch::PageMask);

	llvm::ReturnInst::Create(fn->getContext(), default_block);

	// switch statement
	auto switch_statement = builder.CreateSwitch(page_offset, default_block, fns.size());
	for(auto i : fns) {
		llvm::BasicBlock *block = llvm::BasicBlock::Create(module->getContext(), "", fn);

		llvm::CallInst::Create(fns.at(i.first), {arg_regptr, arg_stateptr}, "", block);
		llvm::ReturnInst::Create(fn->getContext(), block);
		switch_statement->addCase(llvm::ConstantInt::get(i32Ty, i.first), block);
	}

	return fn;
}

LLVMTranslation* AsynchronousTranslationWorker::CompileModule(TranslationWorkUnit& unit, ::llvm::Module* module, llvm::Function* function)
{

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
	jit_symbols["blkWrite8"] = (void*)cpuWrite8;
	jit_symbols["blkWrite16"] = (void*)cpuWrite16;
	jit_symbols["blkWrite32"] = (void*)cpuWrite32;

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
		txln->AddContainedBlock(i.first);
	}

	return txln;
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

	translate::adapt::BlockJITToLLVMAdaptor adaptor(llvm_ctx);

	gensim::blockjit::BaseBlockJITTranslate *translate = translate_;
	translate->InitialiseFeatures(unit.GetThread());
	translate->InitialiseIsaMode(unit.GetThread());
	translate->SetDecodeContext(unit.GetThread()->GetEmulationModel().GetNewDecodeContext(*unit.GetThread()));
	// create a function for each block in the region

	std::map<uint32_t, llvm::Function *> block_map;

	for(auto block : unit.GetBlocks()) {
		captive::arch::jit::TranslationContext blockjit_ctx;
		captive::shared::IRBuilder builder;
		builder.SetBlock(blockjit_ctx.alloc_block());
		builder.SetContext(&blockjit_ctx);

		for(auto insn : block.second->GetInstructions()) {
			translate->emit_instruction_decoded(unit.GetThread(), Address(unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset()), &insn->GetDecode(), builder);
		}
		builder.ret();

		auto fn = adaptor.AdaptIR(unit.GetThread(), module, "block_" + std::to_string(block.first.Get()), blockjit_ctx);
		block_map[block.first.Get()] = fn;
	}

	// build a dispatch function
	auto dispatch_function_ = BuildDispatchFunction(module, unit, block_map);

	// optimise
	opt.Optimise(module, target_machine_->createDataLayout());

	// compile
	auto txln = CompileModule(unit, module, dispatch_function_);

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
