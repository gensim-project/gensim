/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/BlockJITExecutionEngine.h"
#include "core/thread/ThreadInstance.h"
#include "core/MemoryInterface.h"
#include "util/LogContext.h"
#include "system.h"

UseLogContext(LogBlockJitCpu);

using namespace archsim::core::execution;
using namespace archsim::core::thread;

extern int block_trampoline_source(ThreadInstance *thread, const archsim::RegisterFileEntryDescriptor &pc_descriptor, struct archsim::blockjit::BlockCacheEntry *block_cache);

BlockJITExecutionEngine::BlockJITExecutionEngine() : phys_block_profile_(mem_allocator_)
{

}


void BlockJITExecutionEngine::checkFlushTxlns()
{
	LC_ERROR(LogBlockJitCpu) << "Not implemented!";
}


ExecutionResult BlockJITExecutionEngine::ArchStepBlock(ThreadInstance* thread)
{
	checkFlushTxlns();
	if(thread->HasMessage()) {
		thread->HandleMessage();
	}
	
	const auto &pc_desc = thread->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");
	
	if(virt_block_cache_.Contains(thread->GetPC()) || translateBlock(thread, thread->GetPC(), true, false)) {
		block_trampoline_source(thread, pc_desc, virt_block_cache_.GetPtr());
	}
}

ExecutionResult BlockJITExecutionEngine::ArchStepSingle(ThreadInstance* thread)
{
	UNIMPLEMENTED;
}

bool BlockJITExecutionEngine::translateBlock(ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling)
{
	// Look up the block in the cache, just in case we already have it translated
	captive::shared::block_txln_fn fn;
	if((fn = virt_block_cache_.Lookup(block_pc))) return true;

	// we missed in the block cache, so fall back to the physical profile
	Address physaddr (0);

	// Translate WITH side effects. If a fault occurs, stop translating this block
	TranslationResult fault = thread->GetFetchMI().PerformTranslation(block_pc, physaddr, thread->GetExecutionRing());

	if(fault != TranslationResult::OK) {
		thread->TakeMemoryException(thread->GetFetchMI(), block_pc);
		return false;
	}

	archsim::blockjit::BlockTranslation txln;
	if(phys_block_profile_.Get(physaddr, thread->GetFeatures(), txln)) {
		virt_block_cache_.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
		return true;
	}

	// we couldn't find the block in the physical profile, so create a new translation
	thread->GetEmulationModel().GetSystem().GetProfileManager().MarkRegionAsCode(PhysicalAddress(physaddr.PageBase().Get()));

	LC_DEBUG4(LogBlockJitCpu) << "Translating block " << std::hex << block_pc.Get();
	auto *translate = translator_;
	if(!support_chaining) translate->setSupportChaining(false);

	if(support_profiling) {
		translate->setSupportProfiling(true);
		translate->setTranslationMgr(&thread->GetEmulationModel().GetSystem().GetTranslationManager());
	}

	bool success = translate->translate_block(thread, block_pc, txln, mem_allocator_);

	if(success) {
		// we successfully created a translation, so add it to the physical profile
		// and to the cache, since we'll probably need it again soon
		phys_block_profile_.Insert(physaddr, txln);
		virt_block_cache_.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
	} else {
		// if we failed to produce a translation, then try and stop the simulation
		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
	}

	return success;
}
