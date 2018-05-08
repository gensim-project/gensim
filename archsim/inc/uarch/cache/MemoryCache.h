/*
 * File:   MemoryCache.h
 * Author: s0457958
 *
 * Created on 25 August 2014, 12:29
 */

#ifndef MEMORYCACHE_H
#define	MEMORYCACHE_H

#include "define.h"
#include "util/Counter.h"
#include "core/thread/ThreadInstance.h"

#include <string>
#include <unordered_map>
#include <ostream>

namespace archsim
{
	namespace uarch
	{
		namespace cache
		{
			class MemoryCacheEventHandler;
			class MemoryCache
			{
				friend class MemoryCacheEventHandler;

			public:
				enum CacheAccessType {
					DataRead,
					DataWrite,
					InstructionFetch
				};

				MemoryCache(std::string name);
				virtual ~MemoryCache();

				inline MemoryCache *GetNext(CacheAccessType accessType) const
				{
					return nextCaches.at(accessType);
				}
				void SetNext(CacheAccessType accessType, MemoryCache& next)
				{
					nextCaches[(uint32_t)accessType] = &next;
				}

				bool Access(archsim::core::thread::ThreadInstance *cpu, CacheAccessType accessType, phys_addr_t phys_addr, virt_addr_t virt_addr, uint8_t size);
				virtual void Flush();

				void PrintStatistics(std::ostream& stream);
				void ResetStatistics()
				{
					hits.clear();
					misses.clear();
				}

				inline uint64_t GetHits() const
				{
					return hits.get_value();
				}
				inline uint64_t GetMisses() const
				{
					return misses.get_value();
				}

				inline const archsim::util::Counter64& GetHitCounter() const
				{
					return hits;
				}
				inline const archsim::util::Counter64& GetMissCounter() const
				{
					return misses;
				}

			protected:
				virtual bool IsHit(CacheAccessType accessType, phys_addr_t phys_addr, virt_addr_t virt_addr, uint8_t size);

			private:
				MemoryCache(const MemoryCache&) = delete;
				MemoryCache& operator=(const MemoryCache&) = delete;

				archsim::util::Counter64 hits;
				archsim::util::Counter64 misses;

				std::unordered_map<uint32_t, MemoryCache *> nextCaches;
				std::string name;

				uint32_t last_block;
			};

			namespace types
			{
				class MainMemory : public MemoryCache
				{
				public:
					MainMemory();
					~MainMemory();

				protected:
					bool IsHit(CacheAccessType accessType, phys_addr_t phys_addr, virt_addr_t virt_addr, uint8_t size);
				};

				class Associative : public MemoryCache
				{
				public:
					enum AddressType {
						Virtual,
						Physical
					};

					enum ReplacementPolicy {
						LRU,
						MRU,
						Random
					};

					Associative(std::string name, uint32_t size, uint8_t ways, uint8_t linesize, ReplacementPolicy rp, AddressType indexType, AddressType tagType);
					~Associative();

					void InvalidateLine(uint8_t way, uint32_t line);
					void InvalidateWay(uint8_t way);

					void Flush() override;

				protected:
					bool IsHit(CacheAccessType accessType, phys_addr_t phys_addr, virt_addr_t virt_addr, uint8_t size);

					uint32_t size;
					uint32_t entries;
					uint32_t entries_per_way;
					uint8_t ways;
					uint8_t linesize;
					uint8_t mru;

					phys_addr_t last_phys_addr;
					virt_addr_t last_virt_addr;

					uint8_t *way_ages;

				private:
					ReplacementPolicy rp;
					AddressType indexType;
					AddressType tagType;

					struct cache_entry {
						uint32_t flags;
						uint32_t tag;
					} __attribute__((packed));

					cache_entry *cache;
				};
			}
		}
	}
}

#endif	/* MEMORYCACHE_H */

