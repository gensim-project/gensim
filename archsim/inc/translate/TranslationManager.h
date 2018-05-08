/*
 * File:   TranslationManager.h
 * Author: s0457958
 *
 * Created on 16 July 2014, 15:10
 */

#ifndef TRANSLATIONMANAGER_H
#define	TRANSLATIONMANAGER_H

#include "define.h"

#include "profile/RegionTable.h"
#include "util/Counter.h"
#include "util/CounterTimer.h"
#include "util/PubSubSync.h"

#include "core/thread/ThreadInstance.h"

#include "translate/TranslationCache.h"
#include "translate/profile/ProfileManager.h"

#include <atomic>
#include <condition_variable>
#include <list>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <set>

#include <cstring>

namespace llvm
{
	class LLVMContext;
}

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class MemoryTranslationModel;
		}
	}
	namespace util
	{
		class CounterTimer;
	}
	namespace translate
	{
		namespace profile
		{
			class Block;
			class Region;
			class RegionTable;
		}
		namespace interrupts
		{
			class InterruptCheckingScheme;
		}

		struct TranslationTimers {
		public:
			util::CounterTimer generation;
			util::CounterTimer optimisation;
			util::CounterTimer compilation;
		};

		class AsynchronousTranslationWorker;
		class TranslationEngine;
		class TranslationWorkUnit;
		class Translation;
		class TranslationManager
		{
		public:
			TranslationManager(util::PubSubContext &pubsubContext);
			virtual ~TranslationManager();

			virtual bool Initialise();
			virtual void Destroy();

			virtual bool TranslateRegion(archsim::core::thread::ThreadInstance *thread, profile::Region& rgn, uint32_t weight) = 0;

			inline bool RegionIsDirty(phys_addr_t region_addr)
			{
				return dirty_code_pages.count(archsim::translate::profile::RegionArch::PageBaseOf(region_addr));
			}

			/**
			 * Profile touched regions and dispatch some for translation if they are hot.
			 * Returns true if regions were dispatched for translation
			 */
			bool Profile(archsim::core::thread::ThreadInstance *thread);

			void RegisterTranslationForGC(Translation& txln);
			void RunGC();

			bool NeedsLeave()
			{
				return _needs_leave;
			}
			void ClearNeedsLeave()
			{
				_needs_leave = 0;
			}

			virtual void UpdateThreshold();

			/**
			 * Completely reset the translation manager
			 */
			void Invalidate();

			void InvalidateRegion(phys_addr_t phys_addr);

			util::Counter64 txln_cache_invalidations;
			void InvalidateRegionTxlnCache();
			void InvalidateRegionTxlnCacheEntry(virt_addr_t virt_addr);

			profile::Region& GetRegion(phys_addr_t phys_addr);

			bool TryGetRegion(phys_addr_t phys_addr, profile::Region*& region);

			void TraceBlock(archsim::core::thread::ThreadInstance *thread, profile::Block& block);

			inline void ResetTrace()
			{
				prev_block.clear();
			}

			inline void **GetRegionTxlnCachePtr()
			{
				if(!txln_cache) return NULL;
				return txln_cache->GetPtr();
			}

			inline void **GetRegionTxlnCacheEntry(virt_addr_t virt_addr)
			{
				if(!txln_cache) return NULL;
				return txln_cache->GetEntry(virt_addr);
			}

			bool MarkTranslationAsComplete(profile::Region &unit, Translation &txln);
			void RegisterCompletedTranslations();

			virtual void PrintStatistics(std::ostream& stream);
			size_t ApproximateRegionMemoryUsage() const;

			profile::ProfileManager &GetManager()
			{
				assert(manager);
				return *manager;
			}
			void SetManager(profile::ProfileManager &manager)
			{
				this->manager = &manager;
			}
		private:
			bool _needs_leave;
			profile::RegionTable regions;

			// ProfileManager keeps track of which pages are code and notifies when pages are invalidated
			profile::ProfileManager *manager;

			TranslationCache *txln_cache;

			std::unordered_map<uint32_t, profile::Block*> prev_block;
			std::unordered_map<uint32_t, uint32_t> region_txln_count;

			std::set<uint32_t> dirty_code_pages;

			std::set<Translation *> stale_txlns;
			std::mutex stale_txlns_lock;

			std::vector<std::pair<profile::Region*, Translation*> > completed_translations;
			std::mutex completed_translations_lock;

			std::atomic<uint32_t> full_invalidations;
			std::atomic<uint32_t> rgn_invalidations;

			std::atomic<uint64_t> total_code;
			std::atomic<uint64_t> current_code;

			util::PubSubscriber subscriber;

			bool RegisterTranslation(profile::Region& unit, Translation& txln);

		protected:
			TranslationEngine *engine;
			interrupts::InterruptCheckingScheme *ics;

			uint32_t curr_hotspot_threshold;
		};

	}
}

#endif	/* TRANSLATIONMANAGER_H */

