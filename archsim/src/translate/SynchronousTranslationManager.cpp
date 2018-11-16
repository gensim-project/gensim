/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * translate/SynchronousTranslationManager.cpp
 *
 * Models a translation manager that translates work units synchronously to the execution of the guest program.
 */
#include "translate/SynchronousTranslationManager.h"
#include "translate/TranslationEngine.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMWorkUnitTranslator.h"
#include "translate/profile/Region.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"


UseLogContext(LogTranslate);

using namespace archsim::translate;

RegisterComponent(TranslationManager, SynchronousTranslationManager, "sync", "Synchronous", archsim::util::PubSubContext*);

/**
 * Default constructor.
 */
SynchronousTranslationManager::SynchronousTranslationManager(archsim::util::PubSubContext *psctx) : TranslationManager(*psctx), llvm_ctx_(), compiler_(llvm_ctx_)
{

}

/**
 * Default destructor.
 */
SynchronousTranslationManager::~SynchronousTranslationManager()
{

}

/**
 * Initialises the synchronous translation manager.
 * @return Returns true if the translation manager was successfully initialised.
 */
bool SynchronousTranslationManager::Initialise(gensim::BaseLLVMTranslate *translator)
{
	if (!TranslationManager::Initialise())
		return false;

	translator_ = translator;
	return true;
}

/**
 * Destroys the synchronous translation manager.
 */
void SynchronousTranslationManager::Destroy()
{
	TranslationManager::Destroy();
}

/**
 * Translates the given region into native code.
 * @param cpu The processor object that will be executing the native code.
 * @param region The profiled region object.
 * @return Returns true if the region was accepted for translation.
 */
bool SynchronousTranslationManager::TranslateRegion(archsim::core::thread::ThreadInstance *thread, profile::Region& region, uint32_t weight)
{
	TranslationWorkUnit *twu = TranslationWorkUnit::Build(thread, region, *ics, weight);
	if (!twu)
		return false;

	region.SetStatus(Region::InTranslation);
	// If debugging is turned on, dump the control-flow graph for this work unit.
	if (archsim::options::Debug) {
		twu->DumpGraph();
	}

// 	LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translating: " << unit;

	translate_llvm::LLVMWorkUnitTranslator txltr(translator_, *llvm_ctx_.getContext());
	auto txlt_result = txltr.TranslateWorkUnit(*twu);

	optimiser_.Optimise(txlt_result.first, txlt_result.first->getDataLayout());

	// compile
	auto handle = compiler_.AddModule(txlt_result.first);
	auto txln = compiler_.GetTranslation(handle, *twu);


	if (!txln) {
// 		LC_ERROR(LogTranslate) << "[" << (uint32_t)id << "] Translation Failed: " << unit;
	} else {
// 		LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translation Succeeded: " << unit;

		if (archsim::options::Verbose) {
// 			txlns.inc();
		}

		if (!MarkTranslationAsComplete(twu->GetRegion(), *txln)) {
			delete txln;
		}
	}

	delete twu;
	return true;
}
