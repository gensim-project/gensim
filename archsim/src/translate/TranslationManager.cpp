/*
 * Copyright (C) University of Edinburgh 2014
 *
 * translate/TranslationManager.cpp
 *
 * The translation manager is responsible for performing a translation of profile guest machine instructions into
 * host native code.
 */
#include "translate/TranslationManager.h"
#include "translate/Translation.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationEngine.h"
#include "translate/profile/Block.h"
#include "translate/profile/Region.h"
#include "translate/profile/RegionTable.h"
#include "translate/interrupts/InterruptCheckingScheme.h"

#include "util/LogContext.h"
#include "util/ComponentManager.h"

#include "gensim/gensim_processor.h"

DeclareLogContext(LogTranslate, "Translate");
DeclareChildLogContext(LogProfile, LogTranslate, "Profile");
UseLogContext(LogLifetime);

using namespace archsim::translate;
using namespace archsim::translate::profile;

DefineComponentType(TranslationManager, archsim::util::PubSubContext*);

RegionTable::RegionTable()
{
	Clear();
}

Region *RegionTable::InstantiateRegion(TranslationManager &mgr, phys_addr_t phys_base)
{
	return new Region(mgr, phys_base);
}

size_t RegionTable::Erase(uint32_t phys_base)
{
	profile::Region *&ptr = regions[profile::RegionArch::PageIndexOf(phys_base)];
	if(!ptr) return 0;
	dense_regions.erase(ptr);

	ptr->Release();
	ptr = NULL;
	return 1;
}

void RegionTable::Clear()
{
	for (auto rgn : dense_regions) {
		rgn->Release();
	}

	dense_regions.clear();
	bzero(regions, sizeof(*regions) * (profile::RegionArch::PageCount));
}

static void InvalidateCallback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	TranslationManager *txln_mgr = (TranslationManager*)ctx;
	switch(type) {
		case PubSubType::ITlbEntryFlush:
			txln_mgr->InvalidateRegionTxlnCacheEntry((uint64_t)data);
			break;
		case PubSubType::ITlbFullFlush:
			txln_mgr->InvalidateRegionTxlnCache();
			break;
		case PubSubType::RegionInvalidatePhysical:
			txln_mgr->InvalidateRegion((uint32_t)(uint64_t)data);
			break;
		case PubSubType::L1ICacheFlush:
			txln_mgr->Invalidate();
			break;
		case PubSubType::FlushTranslations:
			txln_mgr->Invalidate();
			break;
		default:
			assert(false);
	}
}

TranslationManager::TranslationManager(util::PubSubContext *psCtx) : _needs_leave(false), ics(NULL), curr_hotspot_threshold(archsim::options::JitHotspotThreshold), subscriber(psCtx), engine(NULL), manager(NULL)
{
	if (!archsim::options::JitDisableBranchOpt) {
		// Allocate storage for the region translation cache.
		txln_cache = new TranslationCache();
		assert(txln_cache->GetPtr());
	} else {
		txln_cache = NULL;
	}

	subscriber.Subscribe(PubSubType::ITlbEntryFlush, InvalidateCallback, this);
	subscriber.Subscribe(PubSubType::ITlbFullFlush, InvalidateCallback, this);
	subscriber.Subscribe(PubSubType::RegionInvalidatePhysical, InvalidateCallback, this);
	subscriber.Subscribe(PubSubType::FlushTranslations, InvalidateCallback, this);
//	subscriber.Subscribe(PubSubType::L1ICacheFlush, InvalidateCallback, this);
}

TranslationManager::~TranslationManager()
{
	regions.Clear();

	delete ics;

	engine->Destroy();
	delete engine;

	if (txln_cache) {
		// Release the region translation cache memory.
		delete txln_cache;
	}

}

bool TranslationManager::Initialise()
{
	// Attempt to acquire the translation engine
	if (!GetComponentInstance(archsim::options::JitEngine, engine)) {
		LC_ERROR(LogTranslate) << "Unable to create translation engine '" << archsim::options::JitEngine.GetValue() << "'";
		return false;
	}

	if (!engine->Initialise()) {
		LC_ERROR(LogTranslate) << "Unable to initialise translation engine '" << archsim::options::JitEngine.GetValue() << "'";
		return false;
	}

	// Attempt to acquire the interrupt checking scheme.
	if (!GetComponentInstance(archsim::options::JitInterruptScheme, ics)) {
		engine->Destroy();
		delete engine;

		LC_ERROR(LogTranslate) << "Unknown interrupt checking scheme ''";
		return false;
	}

	return true;
}

void TranslationManager::Destroy()
{

}

Region& TranslationManager::GetRegion(phys_addr_t phys_addr)
{
	GetManager().MarkRegionAsCode(PhysicalAddress(phys_addr).PageBase());
	return regions.Get(*this, RegionArch::PageBaseOf(phys_addr));
}

bool TranslationManager::TryGetRegion(phys_addr_t phys_addr, profile::Region*& region)
{
	if(dirty_code_pages.count(archsim::translate::profile::RegionArch::PageBaseOf(phys_addr))) return false;
	return regions.TryGet(*this, RegionArch::PageBaseOf(phys_addr), region);
}

void TranslationManager::TraceBlock(gensim::Processor& cpu, Block& block)
{
	uint32_t mode = (uint32_t)cpu.interrupt_mode;

	LC_DEBUG4(LogProfile) << "Tracing block " << std::hex << block.GetOffset() << ", isa = " << (uint32_t)cpu.get_cpu_mode() << ", mode = " << mode;

	if (prev_block[mode] && prev_block[mode]->GetParent().GetPhysicalBaseAddress() == block.GetParent().GetPhysicalBaseAddress()) {
		prev_block[mode]->AddSuccessor(block);
	} else if (!block.IsRootBlock()) {
		block.SetRoot();
	}

	prev_block[mode] = &block;
}

bool TranslationManager::Profile(gensim::Processor& cpu)
{
	LC_DEBUG3(LogProfile) << "Performing profile";
	bool txltd_regions = false;

	for (auto region : regions) {
		if(dirty_code_pages.count(region->GetPhysicalBaseAddress())) continue;

		if (region->IsHot(curr_hotspot_threshold)) {
			region_txln_count[region->GetPhysicalBaseAddress()]++;

			uint32_t weight = (region->TotalBlockHeat()) / region_txln_count[region->GetPhysicalBaseAddress()];

			region->SetStatus(Region::InTranslation);
			txltd_regions = true;

			cpu.GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::RegionDispatchedForTranslationPhysical, (void*)(uint64_t)region->GetPhysicalBaseAddress());
			for(auto virt_base : region->virtual_images)cpu.GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::RegionDispatchedForTranslationVirtual, (void*)(uint64_t)virt_base);

#ifdef PROTECT_CODE_REGIONS
			host_addr_t addr;
			bool b = cpu.GetEmulationModel().GetMemoryModel().LockRegion(region->GetPhysicalBaseAddress(), 4096, addr);
			mprotect(addr, 4096, PROT_READ);
#endif

			assert(region->IsValid());

			if (!TranslateRegion(cpu, *region, weight)) {
				region->SetStatus(Region::NotInTranslation);
				LC_WARNING(LogTranslate) << "Region translation failed for region " << *region;
			}

			region->InvalidateHeat();
		}
	}

	dirty_code_pages.clear();

	UpdateThreshold();

	prev_block.clear();

	return txltd_regions;
}

void TranslationManager::Invalidate()
{
	full_invalidations++;

	ResetTrace();
	InvalidateRegionTxlnCache();

	for(auto region : regions) {
		region->Invalidate();
		_needs_leave = true;
	}

	dirty_code_pages.clear();
	regions.Clear();


}

void TranslationManager::InvalidateRegion(phys_addr_t phys_addr)
{
	assert((phys_addr & 0xfff) == 0);

	LC_DEBUG2(LogTranslate) << "Invalidating region " << std::hex << phys_addr;

	dirty_code_pages.insert(archsim::translate::profile::RegionArch::PageBaseOf(phys_addr));
	Region& rgn = regions.Get(*this, phys_addr);

	if (txln_cache) {
		txln_cache->InvalidateEntry(rgn.GetPhysicalBaseAddress());
	}

	for(auto &i : prev_block) {
		if(&i.second->GetParent() == &rgn) i.second = NULL;
	}

	rgn.Invalidate();
	regions.Erase(phys_addr);

	rgn_invalidations++;
}

void TranslationManager::InvalidateRegionTxlnCache()
{
	if(txln_cache)
		txln_cache->Invalidate();
}

void TranslationManager::InvalidateRegionTxlnCacheEntry(virt_addr_t virt_addr)
{
	if(txln_cache)
		txln_cache->InvalidateEntry(virt_addr);
}

bool TranslationManager::MarkTranslationAsComplete(Region &unit, Translation &txln)
{
	completed_translations_lock.lock();
	completed_translations.push_back({&unit, &txln});
	completed_translations_lock.unlock();

	return true;
}

void TranslationManager::RegisterCompletedTranslations()
{
	if(completed_translations.empty()) return;
	bool registered = false;

	completed_translations_lock.lock();
	for(auto &txln : completed_translations) {
		registered |= RegisterTranslation(*txln.first, *txln.second);
		InvalidateRegionTxlnCacheEntry(txln.first->GetPhysicalBaseAddress());
	}
	completed_translations.clear();
	completed_translations_lock.unlock();

}

bool TranslationManager::RegisterTranslation(Region& region, Translation& txln)
{
	LC_DEBUG1(LogTranslate) << "Registering translation for " << region;

	total_code += txln.GetCodeSize();

	if (!region.IsValid()) {
		LC_DEBUG1(LogTranslate) << "Discarding TWU (invalid region) " << region;
		return false;
	}

	if (region.txln) {
		RegisterTranslationForGC(*region.txln);
	}

	region.txln = &txln;

	region.SetStatus(Region::NotInTranslation);

	current_code += txln.GetCodeSize();

	return true;
}

void TranslationManager::RegisterTranslationForGC(Translation& txln)
{
	std::unique_lock<std::mutex> lock(stale_txlns_lock);

	LC_DEBUG2(LogLifetime) << "Registering stale translation " << std::hex << &txln;
//	fprintf(stderr, "*** Invalidate %p\n", &txln);
	stale_txlns.insert(&txln);
	current_code -= txln.GetCodeSize();
}

void TranslationManager::RunGC()
{
	std::unique_lock<std::mutex> lock(stale_txlns_lock);

	if (stale_txlns.size() == 0)
		return;

	LC_DEBUG2(LogLifetime) << "Running garbage collector for " << stale_txlns.size() << " stale translations.";

	for (auto txln : stale_txlns) {
		delete txln;
	}

	stale_txlns.clear();
}

void TranslationManager::UpdateThreshold()
{

}

void TranslationManager::PrintStatistics(std::ostream& stream)
{
	stream << "Translation Statistics" << std::endl;

	stream << "  Full Invalidations:   " << full_invalidations << std::endl;
	stream << "  Region Invalidations: " << rgn_invalidations << std::endl;

	uint32_t live_txlns = 0;
	uint32_t live_regions = 0;
	for (auto rgn : regions) {
		live_regions++;
		live_txlns += 0; // TODO: rgn->virtual.size();
	}

	stream << "  Live Regions:         " << live_regions << std::endl;
	stream << "  Live Translations:    " << live_txlns << std::endl;
	stream << "  Total Code:           " << total_code << std::endl;
	stream << "  Final Code:           " << current_code << std::endl;
	stream << std::endl;
}

size_t TranslationManager::ApproximateRegionMemoryUsage() const
{
	size_t total = 0;
	for(auto r : regions) total += r->GetApproximateMemoryUsage();
	return total;
}

Translation::Translation() : is_registered(false)
{

}

Translation::~Translation()
{

}
