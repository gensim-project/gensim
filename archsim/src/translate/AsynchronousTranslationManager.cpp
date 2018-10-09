/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * translate/AsynchronousTranslationManager.cpp
 *
 * Models a translation manager that provides one or more asynchronous worker threads to perform JIT
 * compilation.
 */
#include "translate/AsynchronousTranslationManager.h"
#include "translate/AsynchronousTranslationWorker.h"
#include "translate/TranslationEngine.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/profile/Region.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"

UseLogContext(LogTranslate);
DeclareChildLogContext(LogWorkQueue, LogTranslate, "WorkQueue");

using namespace archsim::translate;

RegisterComponent(TranslationManager, AsynchronousTranslationManager, "async", "Asynchronous", archsim::util::PubSubContext*);

AsynchronousTranslationManager::AsynchronousTranslationManager(util::PubSubContext *psctx) : TranslationManager(*psctx) { }

AsynchronousTranslationManager::~AsynchronousTranslationManager() { }

bool WorkUnitQueueComparator::operator()(const TranslationWorkUnit* lhs, const TranslationWorkUnit* rhs) const
{
	return lhs->GetWeight() < rhs->GetWeight();
}

bool AsynchronousTranslationManager::Initialise(gensim::BaseLLVMTranslate *translate)
{
	if (!TranslationManager::Initialise())
		return false;

	for (unsigned int i = 0; i < archsim::options::JitThreads; i++) {
		auto worker = new AsynchronousTranslationWorker(*this, i, translate);
		workers.push_back(worker);
		worker->start();
	}

	return true;
}

void AsynchronousTranslationManager::Destroy()
{
	// No point notfiying threads here of the change to the queue, as they are
	// about to be terminated.
	work_unit_queue_lock.lock();

	while(!work_unit_queue_.empty()) {
		auto i = work_unit_queue_.back();
		work_unit_queue_.pop_back();
		delete i;
	}

	work_unit_queue_lock.unlock();

	while (!workers.empty()) {
		auto worker = workers.front();
		workers.pop_front();

		worker->stop();
		delete worker;
	}

	TranslationManager::Destroy();
}

bool AsynchronousTranslationManager::UpdateThreshold()
{
	auto initial_threshold = curr_hotspot_threshold;

	if (work_unit_queue_.size() > workers.size() * 2) {
		curr_hotspot_threshold *= 10;

		// Cap the threshold to stop it from overflowing
		uint32_t max_threshold = archsim::options::JitProfilingInterval / 2;
		if (curr_hotspot_threshold > max_threshold) {
			curr_hotspot_threshold = max_threshold;
		}
	} else {
		curr_hotspot_threshold /= 2;

		if (curr_hotspot_threshold < archsim::options::JitHotspotThreshold) {
			curr_hotspot_threshold = archsim::options::JitHotspotThreshold;
		}
	}

	// Since the threshold modifications are multiplicative, if the threshold becomes 0 we will be stuck
	assert(curr_hotspot_threshold > 0);

	return initial_threshold != curr_hotspot_threshold;
}

bool AsynchronousTranslationManager::TranslateRegion(archsim::core::thread::ThreadInstance *cpu, profile::Region& region, uint32_t weight)
{
//	fprintf(stderr, "*** Translating %x\n", region.GetPhysicalBaseAddress());

	if (!region.IsValid()) return false;

	// Create the translation work unit for this region.
	TranslationWorkUnit *twu = TranslationWorkUnit::Build(cpu, region, *ics, weight);
	if (!twu) {
		LC_WARNING(LogTranslate) << "Could not build TWU for " << region;
		return false;
	}

	// Acquire the work unit queue lock.
	work_unit_queue_lock.lock();

	work_unit_queue_.push_back(twu);
	LC_DEBUG1(LogWorkQueue) << "[ENQUEUE] Enqueueing " << *twu << ", queue length " << work_unit_queue_.size() << " threshold " << curr_hotspot_threshold;

	work_unit_queue_cond.notify_one();
	work_unit_queue_lock.unlock();

	return true;
}

void AsynchronousTranslationManager::PrintStatistics(std::ostream& stream)
{
	TranslationManager::PrintStatistics(stream);

	work_unit_queue_lock.lock();
	stream << "Work Queue Size: " << work_unit_queue_.size() << std::endl;
	work_unit_queue_lock.unlock();

	stream << "-----------------------------------------------------" << std::endl;
	stream << "#  Generation    Optimisation  Compilation" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;

	uint32_t total_units = 0, total_txlns = 0;
	for (auto worker : workers) {
		total_units += worker->units.get_value();
		total_txlns += worker->txlns.get_value();

		stream << (uint32_t)worker->id << "  ";
		worker->PrintStatistics(stream);
	}

	stream << "-----------------------------------------------------" << std::endl;

	stream << "  Total Units:        " << total_units << std::endl;
	stream << "  Total Translations: " << total_txlns;
	stream << " (" << ((double)total_txlns / total_units) << " txlns/unit)" << std::endl;
}
