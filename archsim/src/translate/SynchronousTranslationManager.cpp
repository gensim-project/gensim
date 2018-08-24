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
#include "util/ComponentManager.h"
#include "util/LogContext.h"

UseLogContext(LogTranslate);

using namespace archsim::translate;

RegisterComponent(TranslationManager, SynchronousTranslationManager, "sync", "Synchronous", archsim::util::PubSubContext*);

/**
 * Default constructor.
 */
SynchronousTranslationManager::SynchronousTranslationManager(archsim::util::PubSubContext *psctx) : TranslationManager(psctx), llvm_ctx(NULL)
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
bool SynchronousTranslationManager::Initialise()
{
	if (!TranslationManager::Initialise())
		return false;

	// Create an LLVM context for performing translations in.
	llvm_ctx = new ::llvm::LLVMContext();
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
bool SynchronousTranslationManager::TranslateRegion(gensim::Processor& cpu, profile::Region& region, uint32_t weight)
{
	TranslationWorkUnit *twu = TranslationWorkUnit::Build(cpu, region, *ics, weight);
	if (!twu)
		return false;

	// Create a new LLVM translation context.
	llvm::LLVMTranslationContext tctx(*this, *twu, *llvm_ctx, optimiser, code_pool);

	// Perform the translation.
	Translation *txln;
	LC_DEBUG1(LogTranslate) << "Translating " << *twu;
	if (!tctx.Translate(txln, timers)) {
		delete twu;
		return false;
	}

	// Register the new translation.
	MarkTranslationAsComplete(twu->GetRegion(), *txln);

	delete twu;
	return true;
}
