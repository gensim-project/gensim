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
#include "translate/llvm/LLVMWorkUnitTranslator.h"



#include <iostream>
#include <iomanip>
#include <set>

UseLogContext(LogTranslate);
UseLogContext(LogWorkQueue);

using namespace archsim::translate;
using namespace archsim::translate::translate_llvm;


AsynchronousTranslationWorker::AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::BaseLLVMTranslate *translate) :
	Thread("Txln Worker"),
	mgr(mgr),
	terminate(false),
	id(id),
	translate_(translate)

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
		while (mgr.work_unit_queue_.empty() && !terminate) {
			// first do a bit of busy work
			compiler_.GC();

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
			if (mgr.work_unit_queue_.empty()) {
				unit = NULL;
				break;
			}


			auto best_it = mgr.work_unit_queue_.begin();
			uint64_t max_heat = (*best_it)->GetWeight();
			auto it = mgr.work_unit_queue_.begin();
			while(it != mgr.work_unit_queue_.end()) {
				auto this_heat = (*it)->GetWeight();
				if(this_heat > max_heat) {
					max_heat = this_heat;
					best_it = it;
				}
				it++;
			}
			unit = *best_it;
			mgr.work_unit_queue_.erase(best_it);

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

		LC_DEBUG1(LogWorkQueue) << "[DEQUEUE] Dequeueing " << *unit << ", queue length " << mgr.work_unit_queue_.size() << ", @ " << (uint32_t)id;

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

LLVMTranslation* AsynchronousTranslationWorker::CompileModule(TranslationWorkUnit& unit, ::llvm::Module* module, llvm::Function* function)
{
	auto handle = compiler_.AddModule(module);
	return compiler_.GetTranslation(handle, unit);
}

/**
 * Translates the given translation work unit.
 * @param llvm_ctx The LLVM context to perform the translation with.
 * @param unit The unit to translate.
 */
void AsynchronousTranslationWorker::Translate(::llvm::LLVMContext& llvm_ctx, TranslationWorkUnit& unit)
{
	unit.GetRegion().SetStatus(Region::InTranslation);
	// If debugging is turned on, dump the control-flow graph for this work unit.
	if (archsim::options::Debug) {
		unit.DumpGraph();
	}

	if (archsim::options::Verbose) {
		units.inc();
		blocks.inc(unit.GetBlockCount());
	}

	LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translating: " << unit;

	translate_llvm::LLVMWorkUnitTranslator txltr(translate_);
	auto txlt_result = txltr.TranslateWorkUnit(unit);

	optimiser_.Optimise(txlt_result.first, txlt_result.first->getDataLayout());

	// compile
	auto txln = CompileModule(unit, txlt_result.first, txlt_result.second);

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
