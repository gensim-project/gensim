#include "translate/AsynchronousTranslationWorker.h"
#include "translate/AsynchronousTranslationManager.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "blockjit/BlockJitTranslate.h"


#include "util/LogContext.h"
#include "translate/profile/Block.h"

#include "translate/Translation.h"

#include <llvm/IR/LLVMContext.h>

#include <iostream>
#include <iomanip>

UseLogContext(LogTranslate);
UseLogContext(LogWorkQueue);

using namespace archsim::translate;
using namespace archsim::translate::translate_llvm;

AsynchronousTranslationWorker::AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::blockjit::BaseBlockJITTranslate *translate) : Thread("Txln Worker"), mgr(mgr), terminate(false), id(id), optimiser(NULL), translate_(translate)
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

static void BuildDispatchFunction(llvm::Module *module, const std::map<uint32_t, llvm::Function*> *fn) {
	llvm::FunctionType *ftype = llvm::FunctionType::get(voidTy, {i8PtrTy, i8PtrTy});
	llvm::Function *fn = module->getOrInsertFunction("region", ftype);
	llvm::BasicBlock *entryblock = llvm::BasicBlock::Create(module->getContext(), "", fn);
	
	llvm::IRBuilder<> builder(entryblock);
	
	// load PC
	
	// mask page offset
	
	// switch statement
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
	llvm::Module *module = new llvm::Module("region_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress()), llvm_ctx);
	
	translate::adapt::BlockJITToLLVMAdaptor adaptor(llvm_ctx);
	
	gensim::blockjit::BaseBlockJITTranslate *translate = translate_;
	translate->InitialiseFeatures(unit.GetThread());
	translate->InitialiseIsaMode(unit.GetThread());
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
		
		auto fn = adaptor.AdaptIR(unit.GetThread(), module, "block_" + std::to_string(block.first), blockjit_ctx);
		block_map[block.first] = fn;
	}
	
	// build a dispatch function
	bool result = false;
	BuildDispatchFunction(module, block_map);
	
	// compile the constructed module
	
	
	if (terminate || !result) {
		LC_ERROR(LogTranslate) << "[" << (uint32_t)id << "] Translation Failed: " << unit;
	} else {
		LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translation Succeeded: " << unit;

		if (archsim::options::Verbose) {
			txlns.inc();
		}

//		if (!mgr.MarkTranslationAsComplete(unit.GetRegion(), *txln)) {
//			delete txln;
//		}
	}
}

void AsynchronousTranslationWorker::PrintStatistics(std::ostream& stream)
{
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.generation.GetElapsedS();
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.optimisation.GetElapsedS();
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.compilation.GetElapsedS();
	stream << std::endl;
}
