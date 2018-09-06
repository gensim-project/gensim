/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/thread/ThreadInstance.h"
#include "core/execution/BasicJITExecutionEngine.h"
#include "core/MemoryInterface.h"
#include "util/LogContext.h"

#include <chrono>

using namespace archsim::core::execution;

DeclareLogContext(LogBasicJIT, "BasicJIT");

void flush_txlns_callback(PubSubType::PubSubType type, void *context, const void *data)
{
	BasicJITExecutionEngine *engine = (BasicJITExecutionEngine*)context;

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

BasicJITExecutionEngine::BasicJITExecutionEngine(uint64_t max_code_size) : phys_block_profile_(mem_allocator_), subscribed_(false), flush_txlns_(0), flush_all_txlns_(0), max_code_size_(max_code_size)
{

}


void BasicJITExecutionEngine::FlushTxlns()
{
	virt_block_cache_.Invalidate();
	flush_txlns_ = 1;
}

void BasicJITExecutionEngine::FlushAllTxlns()
{
	virt_block_cache_.Invalidate();
	flush_all_txlns_ = 1;
	flush_txlns_ = 1;
}

void BasicJITExecutionEngine::FlushTxlnCache()
{
	virt_block_cache_.Invalidate();
}

void BasicJITExecutionEngine::FlushTxlnsFeature()
{
	for(auto i : GetThreads()) {
		virt_block_cache_.InvalidateFeatures(i->GetFeatures().GetAvailableMask());
	}
}

void BasicJITExecutionEngine::InvalidateRegion(Address addr)
{
	phys_block_profile_.MarkPageDirty(addr);
}

void BasicJITExecutionEngine::checkFlushTxlns()
{
	if(flush_txlns_) {
		if(archsim::options::AggressiveCodeInvalidation || flush_all_txlns_) phys_block_profile_.Invalidate();
		else phys_block_profile_.GarbageCollect();
		virt_block_cache_.Invalidate();
		flush_txlns_ = false;
		flush_all_txlns_ = false;
	}
}

void BasicJITExecutionEngine::checkCodeSize()
{
	if(max_code_size_ == 0) {
		return;
	}
	if(phys_block_profile_.GetTotalCodeSize() > max_code_size_) {
		phys_block_profile_.Invalidate();
		virt_block_cache_.Invalidate();
	}

}


template<typename PC_t> ExecutionResult BasicJITExecutionEngine::ExecuteLoop(ExecutionEngineThreadContext *ctx, PC_t *pc_ptr)
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

		block_txln_fn fn;
		if(lookupBlock(thread, Address(*pc_ptr), fn)) {
			assert(fn != nullptr);

			ExecuteInnerLoop(ctx, pc_ptr);
		} else {
			auto pre_time = std::chrono::high_resolution_clock::now();

			if(!translateBlock(thread, Address(*pc_ptr), false, false)) {
				// failed to decode a block: abort
				return ExecutionResult::Abort;
			}

			auto post_time = std::chrono::high_resolution_clock::now();

			auto duration = post_time - pre_time;

//			std::cout << "Compiled block " << Address(*pc_ptr) << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(post_time-pre_time).count() << "ns" << std::endl;
		}

		if(thread->GetTraceSource() != nullptr && thread->GetTraceSource()->IsPacketOpen()) {
			thread->GetTraceSource()->Trace_End_Insn();
		}
	}

	return ExecutionResult::Halt;
}

template<typename PC_t> void BasicJITExecutionEngine::ExecuteInnerLoop(ExecutionEngineThreadContext* ctx, PC_t* pc_ptr)
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

ExecutionResult BasicJITExecutionEngine::Execute(ExecutionEngineThreadContext* ctx)
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

bool BasicJITExecutionEngine::lookupBlock(thread::ThreadInstance* thread, Address addr, captive::shared::block_txln_fn& txln_fn)
{
	LC_DEBUG2(LogBasicJIT) << "Looking up " << addr;
	// Look up the block in the cache, just in case we already have it translated
	if((txln_fn = virt_block_cache_.Lookup(addr))) {
		LC_DEBUG2(LogBasicJIT) << " - found in cache";
		return true;
	}

	// we missed in the block cache, so fall back to the physical profile
	Address physaddr (0);

	// Translate WITH side effects. If a fault occurs, stop translating this block
	archsim::TranslationResult fault = thread->GetFetchMI().PerformTranslation(addr, physaddr, false, true, true);

	if(fault != archsim::TranslationResult::OK) {
		thread->TakeMemoryException(thread->GetFetchMI(), addr);
		LC_DEBUG2(LogBasicJIT) << " - Memory fault";
		return false;
	}

	archsim::blockjit::BlockTranslation txln = phys_block_profile_.Get(physaddr, thread->GetFeatures());

	LC_DEBUG2(LogBasicJIT) << " - Found translation (" << (void*)txln.GetFn() << "), checking features:";

	if(txln.IsValid(thread->GetFeatures())) {
		LC_DEBUG2(LogBasicJIT) << " - Features valid";
		virt_block_cache_.Insert(addr, txln.GetFn(), txln.GetFeatures());
		txln_fn = txln.GetFn();
		return true;
	} else {
		LC_DEBUG2(LogBasicJIT) << " - Features invalid";
		return false;
	}
}

void BasicJITExecutionEngine::registerTranslation(Address phys_addr, Address virt_addr, archsim::blockjit::BlockTranslation& txln)
{
	LC_DEBUG2(LogBasicJIT) << "Registering translation at " << virt_addr << "(" << phys_addr << ")";
	phys_block_profile_.Insert(phys_addr, txln);
	virt_block_cache_.Insert(virt_addr, txln.GetFn(), txln.GetFeatures());
}
