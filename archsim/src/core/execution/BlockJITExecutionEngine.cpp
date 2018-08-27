/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/lowering/NativeLowering.h"
#include "core/execution/BlockJITExecutionEngine.h"
#include "core/execution/ExecutionEngineFactory.h"
#include "core/thread/ThreadInstance.h"
#include "core/MemoryInterface.h"
#include "util/LogContext.h"
#include "system.h"

#include <setjmp.h>

UseLogContext(LogBlockJitCpu);

using namespace archsim::core::execution;
using namespace archsim::core::thread;


BlockJITExecutionEngine::BlockJITExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator) : translator_(translator)
{

}


ExecutionEngineThreadContext* BlockJITExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

bool BlockJITExecutionEngine::translateBlock(ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling)
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

	LC_DEBUG4(LogBlockJitCpu) << "Translating block " << std::hex << block_pc.Get();
	auto *translate = translator_;
	if(!support_chaining) translate->setSupportChaining(false);

	if(support_profiling) {
		translate->setSupportProfiling(true);
	}

	bool success = translate->translate_block(thread, block_pc, txln, GetMemAllocator());

	if(success) {
		// we successfully created a translation, so add it to the physical profile
		// and to the cache, since we'll probably need it again soon
		registerTranslation(physaddr, block_pc, txln);
	} else {
		// if we failed to produce a translation, then try and stop the simulation
		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
		return false;
	}

	return success;
}

ExecutionEngine *archsim::core::execution::BlockJITExecutionEngine::Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix)
{
	std::string entry_name = cpu_prefix + "BlockJITTranslator";
	if(!module->HasEntry(entry_name)) {
		LC_ERROR(LogBlockJitCpu) << "Could not find BlockJITTranslator in target module";
		return nullptr;
	}

	if(!captive::arch::jit::lowering::HasNativeLowering()) {
		LC_ERROR(LogBlockJitCpu) << "No native lowering found";
		return nullptr;
	}

	auto translator_entry = module->GetEntry<archsim::module::ModuleBlockJITTranslatorEntry>(entry_name)->Get();
	return new BlockJITExecutionEngine(translator_entry);
}

static archsim::core::execution::ExecutionEngineFactoryRegistration registration("BlockJIT", 100, archsim::core::execution::BlockJITExecutionEngine::Factory);
