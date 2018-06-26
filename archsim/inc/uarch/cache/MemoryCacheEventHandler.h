/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   MemoryCacheEventHandler.h
 * Author: s0457958
 *
 * Created on 25 August 2014, 15:10
 */

#ifndef MEMORYCACHEEVENTHANDLER_H
#define	MEMORYCACHEEVENTHANDLER_H

#include "abi/memory/MemoryModel.h"
#include "abi/memory/MemoryEventHandler.h"
#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "uarch/cache/MemoryCache.h"

#include <ostream>
#include <unordered_set>

namespace llvm
{
	class Value;
}

namespace archsim
{
	namespace uarch
	{
		namespace cache
		{
			class MemoryCacheEventHandler : public archsim::abi::memory::MemoryEventHandler, public archsim::uarch::cache::MemoryCache
			{
			public:
				MemoryCacheEventHandler();

				bool HandleEvent(archsim::core::thread::ThreadInstance* cpu, archsim::abi::memory::MemoryModel::MemoryEventType type, Address addr, uint8_t size) override;
				void PrintGlobalCacheStatistics(std::ostream& stream);
				bool IsHit(CacheAccessType accessType, Address phys_addr, Address virt_addr, uint8_t size) override;

				archsim::abi::memory::MemoryEventHandlerTranslator& GetTranslator() override
				{
#if CONFIG_LLVM
					return translator;
#else
					assert(false);
#endif
				}

			private:
#if CONFIG_LLVM
				archsim::abi::memory::DefaultMemoryEventHandlerTranslator translator;
#endif

				void PrintStatisticsRecursive(std::ostream& stream, std::unordered_set<MemoryCache *>& seen, MemoryCache& cache);
			};
		}
	}
}

#endif	/* MEMORYCACHEEVENTHANDLER_H */

