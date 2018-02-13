/*
 * gensim_processor_blockjit.cpp
 *
 *  Created on: 24 Aug 2015
 *      Author: harry
 */

#include <sys/mman.h>
#include "gensim/gensim_processor_blockjit.h"
#include "abi/memory/system/SystemMemoryModel.h"
#include "util/LogContext.h"
#include "translate/profile/Region.h"
#include "translate/profile/Block.h"
#include "translate/Translation.h"
#include "abi/devices/MMU.h"
#include "util/ComponentManager.h"
#include "gensim/gensim_decode_context.h"

#include <libtrace/TraceSource.h>

UseLogContext(LogBlockJit);
DeclareChildLogContext(LogBlockJitCpu, LogCPU, "BlockJit")

extern "C" bool block_trampoline(void *, void *);
extern "C" bool block_trampoline_chain(void *, void *);

using namespace gensim;
using namespace captive::arch::jit;

using namespace archsim::translate;
using namespace archsim::translate::profile;

extern int block_trampoline_source(struct cpuState *state, struct archsim::blockjit::BlockCacheEntry *block_cache, uint32_t*);

static void tlbflush_callback(PubSubType::PubSubType type, void *context, const void *data)
{
	BlockJitProcessor *cpu = (BlockJitProcessor*)context;
	switch(type) {
		case PubSubType::ITlbEntryFlush:
		case PubSubType::ITlbFullFlush:
			cpu->FlushTxlnCache();
			break;
		case PubSubType::L1ICacheFlush:
		case PubSubType::FlushTranslations:
			cpu->FlushTxlns();
			break;
		case PubSubType::FlushAllTranslations:
			cpu->FlushAllTxlns();
			break;
		case PubSubType::RegionInvalidatePhysical:
			cpu->InvalidatePage(archsim::PhysicalAddress((uint32_t)(uint64_t)data));
			break;
		case PubSubType::FeatureChange:
			cpu->FlushTxlnsFeature();
			break;
		default:
			assert(false);
			break;
	}


}

BlockJitProcessor::BlockJitProcessor(const std::string &arch_name, int core_id, archsim::util::PubSubContext* pubsub) : Processor(arch_name, core_id, pubsub), _translator(NULL), _phys_block_profile(_block_allocator), _flush_txlns(false), _flush_all_txlns(false), _pc_ptr(nullptr)
{


}

BlockJitProcessor::~BlockJitProcessor()
{

}

bool BlockJitProcessor::Initialise(archsim::abi::EmulationModel& emulation_model, archsim::abi::memory::MemoryModel& memory_model)
{
	if(!Processor::Initialise(emulation_model, memory_model)) {
		return false;
	}

	GetSubscriber()->Subscribe(PubSubType::RegionInvalidatePhysical, tlbflush_callback, (void*)this);
	GetSubscriber()->Subscribe(PubSubType::L1ICacheFlush, tlbflush_callback, (void*)this);
	GetSubscriber()->Subscribe(PubSubType::ITlbFullFlush, tlbflush_callback, (void*)this);
	GetSubscriber()->Subscribe(PubSubType::ITlbEntryFlush, tlbflush_callback, (void*)this);
	GetSubscriber()->Subscribe(PubSubType::FeatureChange, tlbflush_callback, (void*)this);
	GetSubscriber()->Subscribe(PubSubType::FlushTranslations, tlbflush_callback, (void*)this);
	GetSubscriber()->Subscribe(PubSubType::FlushAllTranslations, tlbflush_callback, (void*)this);

	_translator = CreateBlockJITTranslate();

	if(!GetComponentInstance(GetArchName(), decode_translate_context_)) {
		LC_ERROR(LogCPU) << "Could not find a decode translate context for arch \"" << GetArchName() << "\"";
		return false;
	}

	return true;
}

void BlockJitProcessor::InitPcPtr()
{
	auto *reg = GetRegisterByTag("PC");
	if(reg) _pc_ptr = (uint32_t*)reg->DataStart;
	else _pc_ptr = nullptr;
}

bool BlockJitProcessor::RunInterp(uint32_t steps)
{
	InitPcPtr();

	interp_time.Start();
	exec_time.Start();
	LC_DEBUG1(LogBlockJitCpu) << "Running blockjit jit";
	captive::shared::block_txln_fn fn = nullptr;
	bool hit_safepoint = false;

	CreateSafepoint();
	if(last_exception_action == archsim::abi::AbortSimulation) {
		interp_time.Stop();
		exec_time.Stop();
		return false;
	}
	state.jit_entry_table = (jit_region_fn_table_ptr)_virt_block_cache.GetPtr();

	const auto &pc_register = GetRegisterByTag("PC");
	assert(pc_register->RegisterWidth == 4);
	uint32_t *pc_ptr = (uint32_t*)(pc_register->DataStart);

	// Really dumb jit for now
	while(!halted) {

		if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_End_Insn();

		// If we should flush translations for some reason, do it now (while
		// we're not trying to execute one)
		if(_flush_txlns) {
			if(archsim::options::AggressiveCodeInvalidation || _flush_all_txlns) _phys_block_profile.Invalidate();
			else _phys_block_profile.GarbageCollect();
			_virt_block_cache.Invalidate();
			_flush_txlns = false;
			_flush_all_txlns = false;
		}

		// Handle any actions such as interrupts
		handle_pending_action();

		// look up a translation in the block cache (or create a new one) and
		// execute it
		if(_virt_block_cache.Contains(archsim::VirtualAddress(read_pc_fast())) || translate_block(archsim::VirtualAddress(read_pc_fast()), true, false)) {
			cur_exec_mode = kExecModeNative;
			block_trampoline_source(&state, _virt_block_cache.GetPtr(), pc_ptr);
			cur_exec_mode = kExecModeInterpretive;
		}

	}

	if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) {
		GetTraceManager()->Trace_End_Insn();
		GetTraceManager()->EmitPackets();
	}

	interp_time.Stop();
	exec_time.Stop();
	return true;
}

bool BlockJitProcessor::step_block_fast()
{
	assert(false);
}

bool BlockJitProcessor::step_block_trace()
{
	// Tracing is handled properly by fast version of function
	return step_block_fast();
}

bool BlockJitProcessor::translate_block(archsim::VirtualAddress block_pc, bool support_chaining, bool support_profiling)
{
	// Look up the block in the cache, just in case we already have it translated
	captive::shared::block_txln_fn fn;
	if((fn = _virt_block_cache.Lookup(block_pc))) return true;

	// we missed in the block cache, so fall back to the physical profile
	uint32_t phys_addr;

	// Translate WITH side effects. If a fault occurs, stop translating this block
	uint32_t fault = GetMemoryModel().PerformTranslation(block_pc.Get(), phys_addr, MMUACCESSINFO_SE(in_kernel_mode(), 0, 1));

	if(fault) {
		handle_fetch_fault(fault);
		return false;
	}

	archsim::PhysicalAddress physaddr(phys_addr);

	archsim::blockjit::BlockTranslation txln;
	if(_phys_block_profile.Get(physaddr, GetFeatures(), txln)) {
		_virt_block_cache.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
		return true;
	}

	// we couldn't find the block in the physical profile, so create a new translation
	GetEmulationModel().GetSystem().GetProfileManager().MarkRegionAsCode(physaddr.PageBase());

	LC_DEBUG1(LogBlockJitCpu) << "Translating block " << std::hex << block_pc.Get();
	auto *translate = _translator;
	if(!support_chaining) translate->setSupportChaining(false);

	if(support_profiling) {
		translate->setSupportProfiling(true);
		translate->setTranslationMgr(&GetEmulationModel().GetSystem().GetTranslationManager());
	}

	if(archsim::options::Verbose)compile_time.Start();

	bool success = translate->translate_block(this, block_pc, txln, _block_allocator);
	if(archsim::options::Verbose)compile_time.Stop();

	if(success) {
		// we successfully created a translation, so add it to the physical profile
		// and to the cache, since we'll probably need it again soon
		_phys_block_profile.Insert(physaddr, txln);
		_virt_block_cache.Insert(block_pc, txln.GetFn(), txln.GetFeatures());
	} else {
		// if we failed to produce a translation, then try and stop the simulation
		LC_ERROR(LogBlockJitCpu) << "Failed to compile block! Aborting.";
		halted = true;
	}

	return success;
}

void BlockJitProcessor::FlushTxlnCache()
{
	_virt_block_cache.Invalidate();
}

void BlockJitProcessor::FlushTxlnsFeature()
{
	_virt_block_cache.InvalidateFeatures(GetFeatures().GetAvailableMask());
}

void BlockJitProcessor::FlushTxlns()
{
	_virt_block_cache.Invalidate();
	_flush_txlns = 1;
}

void BlockJitProcessor::FlushAllTxlns()
{
	_virt_block_cache.Invalidate();
	_flush_all_txlns = 1;
	_flush_txlns = 1;
}


bool BlockJitProcessor::GetTranslatedBlock(archsim::PhysicalAddress address, const archsim::ProcessorFeatureSet &features, archsim::blockjit::BlockTranslation &txln)
{
	return _phys_block_profile.Get(address, features, txln);
}

void BlockJitProcessor::InvalidatePage(archsim::PhysicalAddress pagebase)
{
	using archsim::translate::profile::RegionArch;
	LC_DEBUG1(LogBlockJitCpu) << "Invalidated page " << std::hex << RegionArch::PageBaseOf(pagebase.Get());

	_phys_block_profile.MarkPageDirty(pagebase);
}

extern "C" void jit_verify(void *cpu)
{
	BlockJitProcessor *processor = (BlockJitProcessor*)cpu;

	processor->GetEmulationModel().GetSystem().CheckVerify();
}
