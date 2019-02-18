/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/TranslationManager.h"
#include "translate/Translation.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationEngine.h"
#include "translate/profile/Block.h"
#include "translate/profile/Region.h"
#include "translate/profile/RegionTable.h"
#include "translate/interrupts/InterruptCheckingScheme.h"

#include "system.h"

#include "util/LogContext.h"
#include "util/ComponentManager.h"

DeclareLogContext(LogTranslate, "Translate");
DeclareChildLogContext(LogProfile, LogTranslate, "Profile");
UseLogContext(LogLifetime);

using archsim::Address;
using namespace archsim::translate;
using namespace archsim::translate::profile;

DefineComponentType(TranslationManager, archsim::util::PubSubContext*);

RegionTable::RegionTable()
{
	Clear();
}

Region *RegionTable::InstantiateRegion(TranslationManager &mgr, Address phys_base)
{
	return new Region(mgr, phys_base);
}

size_t RegionTable::Erase(Address phys_base)
{
	profile::Region *&ptr = regions[phys_base.GetPageIndex()];
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
			txln_mgr->InvalidateRegionTxlnCacheEntry(Address((uint64_t)data));
			break;
		case PubSubType::ITlbFullFlush:
			txln_mgr->InvalidateRegionTxlnCache();
			break;
		case PubSubType::RegionInvalidatePhysical:
			txln_mgr->InvalidateRegion(Address((uint64_t)data));
			break;
		case PubSubType::L1ICacheFlush:
			txln_mgr->Invalidate();
			break;
		case PubSubType::FlushAllTranslations:
			txln_mgr->Invalidate();
			break;
		case PubSubType::FlushTranslations:
			txln_mgr->InvalidateRegionTxlnCache();
			break;
		default:
			assert(false);
	}
}

TranslationManager::TranslationManager(util::PubSubContext &psCtx) : _needs_leave(false), ics(NULL), curr_hotspot_threshold(archsim::options::JitHotspotThreshold), subscriber(psCtx), manager(NULL)
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
	subscriber.Subscribe(PubSubType::FlushAllTranslations, InvalidateCallback, this);
//	subscriber.Subscribe(PubSubType::L1ICacheFlush, InvalidateCallback, this);
}

TranslationManager::~TranslationManager()
{
	regions.Clear();

	delete ics;

	if (txln_cache) {
		// Release the region translation cache memory.
		delete txln_cache;
	}

}

bool TranslationManager::TranslateRegion(archsim::core::thread::ThreadInstance* thread, profile::Region& rgn, uint32_t weight)
{
	return false;
}


bool TranslationManager::Initialise()
{

	// Attempt to acquire the interrupt checking scheme.
	if (!GetComponentInstance(archsim::options::JitInterruptScheme, ics)) {
		LC_ERROR(LogTranslate) << "Unknown interrupt checking scheme ''";
		return false;
	}

	return true;
}

void TranslationManager::Destroy()
{

}

Region& TranslationManager::GetRegion(Address phys_addr)
{
	auto &cache_entry = region_cache_.GetEntry(phys_addr);
	if(cache_entry.tag != phys_addr.GetPageIndex()) {
		GetCodeRegions().MarkRegionAsCode(PhysicalAddress(phys_addr.GetPageBase()));
		auto &region = regions.Get(*this, phys_addr.PageBase());

		cache_entry.tag = phys_addr.GetPageIndex();
		cache_entry.data = &region;
	}

	touched_regions_.insert(cache_entry.data);

	return *cache_entry.data;
}

bool TranslationManager::TryGetRegion(Address phys_addr, profile::Region*& region)
{
	if(dirty_code_pages.count(phys_addr.GetPageBase())) return false;
	return regions.TryGet(*this, phys_addr.PageBase(), region);
}

void TranslationManager::TraceBlock(archsim::core::thread::ThreadInstance *thread, Block& block)
{
	uint32_t mode = (uint32_t)thread->GetExecutionRing();

	LC_DEBUG4(LogProfile) << "Tracing block " << std::hex << block.GetOffset() << ", isa = " << (uint32_t)thread->GetModeID() << ", mode = " << mode;
	auto &prev_block = this->prev_block[mode];

	if (prev_block && prev_block->GetParent().GetPhysicalBaseAddress() == block.GetParent().GetPhysicalBaseAddress()) {
		prev_block->AddSuccessor(block);
	} else if (!block.IsRootBlock()) {
		block.SetRoot();
	}

	prev_block = &block;
}

bool archsim::translate::TranslationManager::ProfileRegion(archsim::core::thread::ThreadInstance* thread, profile::Region* region)
{
	if(dirty_code_pages.count(region->GetPhysicalBaseAddress().Get())) {
		return false;
	}

	if (region->IsHot(curr_hotspot_threshold)) {
		region_txln_count[region->GetPhysicalBaseAddress().Get()]++;

		uint32_t weight = (region->TotalBlockHeat()) / region_txln_count[region->GetPhysicalBaseAddress().Get()];

		region->SetStatus(Region::InTranslation);

		thread->GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::RegionDispatchedForTranslationPhysical, (void*)(uint64_t)region->GetPhysicalBaseAddress().Get());
		for(auto virt_base : region->virtual_images) {
			thread->GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::RegionDispatchedForTranslationVirtual, (void*)virt_base.Get());
		}

#ifdef PROTECT_CODE_REGIONS
		host_addr_t addr;
		bool b = cpu.GetEmulationModel().GetMemoryModel().LockRegion(region->GetPhysicalBaseAddress(), 4096, addr);
		mprotect(addr, 4096, PROT_READ);
#endif

		assert(region->IsValid());

		if (!TranslateRegion(thread, *region, weight)) {
			region->SetStatus(Region::NotInTranslation);
			LC_WARNING(LogTranslate) << "Region translation failed for region " << *region;
		}

		region->InvalidateHeat();

		return true;
	}

	return false;
}

bool TranslationManager::Profile(archsim::core::thread::ThreadInstance *thread)
{
	LC_DEBUG3(LogProfile) << "Performing profile";
	bool txltd_regions = false;

	if(UpdateThreshold()) {
		for(auto region : regions) {
			txltd_regions |= ProfileRegion(thread, region);
		}
	} else {
		for(auto region : touched_regions_) {
			txltd_regions |= ProfileRegion(thread, region);
		}
	}

	touched_regions_.clear();
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

	// invalidate cache
	region_cache_.Invalidate();
}

void TranslationManager::InvalidateRegion(Address phys_addr)
{
	assert((phys_addr.Get() & 0xfff) == 0);

	LC_DEBUG2(LogTranslate) << "Invalidating region " << std::hex << phys_addr;

	dirty_code_pages.insert(phys_addr.GetPageBase());
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

void TranslationManager::InvalidateRegionTxlnCacheEntry(Address virt_addr)
{
	if(txln_cache)
		txln_cache->InvalidateEntry(virt_addr);
}

bool TranslationManager::MarkTranslationAsComplete(Region &unit, Translation &txln)
{
	completed_translations_lock.lock();
	unit.Acquire();
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
		txln.first->Release();
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

bool TranslationManager::UpdateThreshold()
{
	return false;
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

Translation::Translation() : is_registered(false), contained_blocks(4096)
{

}

Translation::~Translation()
{

}
