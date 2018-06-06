/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

void flush_txlns_callback(PubSubType::PubSubType type, void *context, const void *data) 
{
	BlockJITExecutionEngine *engine = (BlockJITExecutionEngine*)context;

	switch(type) {
		case PubSubType::ITlbFullFlush:
		case PubSubType::ITlbEntryFlush:
			engine->FlushTxlnCache();
			break;
		case PubSubType::FlushTranslations:
		case PubSubType::L1ICacheFlush:
			engine->FlushTxlns();
			break;
		
		case PubSubType::FeatureChange:
			engine->FlushTxlnsFeature();
			break;
			
		case PubSubType::FlushAllTranslations:
			engine->FlushAllTxlns();
			break;
			
		case PubSubType::RegionInvalidatePhysical:
			engine->InvalidateRegion(archsim::Address((uint64_t)data));
			break;
		default:
			break;
	}
}

BlockJITExecutionEngine::BlockJITExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator) : phys_block_profile_(mem_allocator_), translator_(translator), subscribed_(false), flush_txlns_(0), flush_all_txlns_(0)
{
	
}

void BlockJITExecutionEngine::FlushTxlns()
{
	virt_block_cache_.Invalidate();
	flush_txlns_ = 1;
}

void BlockJITExecutionEngine::FlushAllTxlns()
{
	virt_block_cache_.Invalidate();
	flush_all_txlns_ = 1;
	flush_txlns_ = 1;
}

void BlockJITExecutionEngine::FlushTxlnCache()
{
	virt_block_cache_.Invalidate();
}

void BlockJITExecutionEngine::FlushTxlnsFeature()
{
	for(auto i : GetThreads()) {
		virt_block_cache_.InvalidateFeatures(i->GetFeatures().GetAvailableMask());
	}	
}

void BlockJITExecutionEngine::InvalidateRegion(Address addr)
{
	phys_block_profile_.MarkPageDirty(addr);
}

void BlockJITExecutionEngine::checkFlushTxlns()
{
	if(flush_txlns_) {
		if(archsim::options::AggressiveCodeInvalidation || flush_all_txlns_) phys_block_profile_.Invalidate();
		else phys_block_profile_.GarbageCollect();
		virt_block_cache_.Invalidate();
		flush_txlns_ = false;
		flush_all_txlns_ = false;
	}
}

template<typename PC_t> ExecutionResult BlockJITExecutionEngine::ExecuteLoop(ExecutionEngineThreadContext *ctx, PC_t *pc_ptr)
{
	auto thread = ctx->GetThread();
	
	CreateThreadExecutionSafepoint(thread);
	
	while(ctx->GetState() == ExecutionState::Running) {
		if(thread->GetTraceSource() && thread->GetTraceSource()->IsPacketOpen()) {
			thread->GetTraceSource()->Trace_End_Insn();
		}
		
		checkFlushTxlns();
		
		if(thread->HasMessage()) {
			auto result = thread->HandleMessage();
			switch(result) {
				case ExecutionResult::Continue: 
				case ExecutionResult::Exception:
					break;
				default:
					return result;
			}
		}
		
		block_txln_fn fn = virt_block_cache_.Lookup(Address(*pc_ptr));
		
		if(!fn) {
			if(!translateBlock(thread, Address(*pc_ptr), true, false)) {
				// Failed to translate a block so halt
				break;
			}
			fn = virt_block_cache_.Lookup(Address(*pc_ptr));
			assert(fn != nullptr);
		}
		
		ExecuteInnerLoop(ctx, pc_ptr);
		
		if(thread->GetTraceSource() != nullptr && thread->GetTraceSource()->IsPacketOpen()) {
			thread->GetTraceSource()->Trace_End_Insn();
		}
	}
	
	return ExecutionResult::Halt;
}

template<typename PC_t> void BlockJITExecutionEngine::ExecuteInnerLoop(ExecutionEngineThreadContext* ctx, PC_t* pc_ptr)
{
	auto thread = ctx->GetThread();
	auto regfile = thread->GetRegisterFile();
	
	while(!thread->HasMessage()) {
		uint64_t pc = *(PC_t*)(pc_ptr);

		uint64_t entry_idx = pc & BLOCKCACHE_MASK;
		entry_idx >>= BLOCKCACHE_INSTRUCTION_SHIFT;

		const auto & entry  = virt_block_cache_.GetEntry(Address(pc));

		if(entry.virt_tag == pc) {
			entry.ptr(regfile, thread->GetStateBlock().GetData());
		} else {
			return;
		}
	}
}



ExecutionResult BlockJITExecutionEngine::Execute(ExecutionEngineThreadContext* ctx)
{
	auto thread = ctx->GetThread();
	
	util::PubSubscriber pubsub (thread->GetPubsub());
	pubsub.Subscribe(PubSubType::FlushTranslations, flush_txlns_callback, this);
	pubsub.Subscribe(PubSubType::FlushAllTranslations, flush_txlns_callback, this);
	pubsub.Subscribe(PubSubType::ITlbFullFlush, flush_txlns_callback, this);
	pubsub.Subscribe(PubSubType::ITlbEntryFlush, flush_txlns_callback, this);
	pubsub.Subscribe(PubSubType::L1ICacheFlush, flush_txlns_callback, this);
	pubsub.Subscribe(PubSubType::FeatureChange, flush_txlns_callback, this);
	pubsub.Subscribe(PubSubType::RegionInvalidatePhysical, flush_txlns_callback, this);
	
	ctx->GetThread()->GetStateBlock().AddBlock("BlockCache", sizeof(void*));
	ctx->GetThread()->GetStateBlock().SetEntry<archsim::blockjit::BlockCacheEntry*>("BlockCache", virt_block_cache_.GetPtr());
	
	std::unique_ptr<util::CounterTimerContext> timer_ctx;
	
	if(archsim::options::Verbose) {
		timer_ctx = std::unique_ptr<util::CounterTimerContext>(new util::CounterTimerContext(thread->GetMetrics().SelfRuntime));
	}
	
	const auto &pc_desc = thread->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");
	void *pc_ptr = (uint8_t*)thread->GetRegisterFile() + pc_desc.GetOffset();
	
	switch(pc_desc.GetEntrySize()) {
		case 4:
			return ExecuteLoop(ctx, (uint32_t*)pc_ptr);
		case 8:
			return ExecuteLoop(ctx, (uint64_t*)pc_ptr);
		default:
			throw std::logic_error("Unsupported PC type");
	}
}

ExecutionEngineThreadContext* BlockJITExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

bool BlockJITExecutionEngine::translateBlock(ThreadInstance *thread, archsim::Address block_pc, bool support_chaining, bool support_profiling)
{
	if(phys_block_profile_.GetTotalCodeSize() > 1024 * 1024 * 64) {
		phys_block_profile_.Invalidate();
		virt_block_cache_.Invalidate();
	}
	
	// Look up the block in the cache, just in case we already have it translated
	captive::shared::block_txln_fn fn;
	if((fn = virt_block_cache_.Lookup(block_pc))) return true;

	// we missed in the block cache, so fall back to the physical profile
	Address physaddr (0);

	// Translate WITH side effects. If a fault occurs, stop translating this block
	TranslationResult fault = thread->GetFetchMI().PerformTranslation(block_pc, physaddr, false, true, true);

	if(fault != TranslationResult::OK) {
		thread->TakeMemoryException(thread->GetFetchMI(), block_pc);
		return false;
	}

	archsim::blockjit::BlockTranslation txln = phys_block_profile_.Get(physaddr, thread->GetFeatures());
	if(txln.IsValid(thread->GetFeatures())) {
		virt_block_cache_.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
		return true;
	}

	// we couldn't find the block in the physical profile, so create a new translation
	thread->GetEmulationModel().GetSystem().GetCodeRegions().MarkRegionAsCode(PhysicalAddress(physaddr.PageBase().Get()));

	LC_DEBUG4(LogBlockJitCpu) << "Translating block " << std::hex << block_pc.Get();
	auto *translate = translator_;
	if(!support_chaining) translate->setSupportChaining(false);

	if(support_profiling) {
		translate->setSupportProfiling(true);
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
		return false;
	}

	return success;
}

ExecutionEngine *archsim::core::execution::BlockJITExecutionEngine::Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix) {
	std::string entry_name = cpu_prefix + "BlockJITTranslator";
	if(!module->HasEntry(entry_name)) {
		return nullptr;
	}
	
	auto translator_entry = module->GetEntry<archsim::module::ModuleBlockJITTranslatorEntry>(entry_name)->Get();
	return new BlockJITExecutionEngine(translator_entry);
}

static archsim::core::execution::ExecutionEngineFactoryRegistration registration("BlockJIT", 100, archsim::core::execution::BlockJITExecutionEngine::Factory);
