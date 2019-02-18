/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Cache.h
 * Author: s0803652
 *
 * Created on 19 October 2011, 12:02
 */

/*
 * File:   Cache.h
 * Author: s0803652
 *
 * Created on 26 August 2011, 11:55
 */

#ifndef _CACHE_H
#define _CACHE_H

#include <cstdint>
#include <bitset>

namespace archsim
{
	namespace util
	{

		typedef enum {
			CACHE_HIT_WAY_0 = 0,
			CACHE_HIT_WAY_1 = 1,
			CACHE_MISS,
			CACHE_INVALID
		} CacheHitType;

		template <typename KeyT, class T, int way_count = 2>
		struct CacheEntry {
		public:
			KeyT tags[way_count];
			T ways[way_count];

			CacheEntry()
			{
				for (int i = 0; i < way_count; ++i) {
					ways[i] = T();
					tags[i] = KeyT(1);
				}
			}

			void purge(std::function<void(T&)> purge_way)
			{
				for (int i = 0; i < way_count; ++i) {
					purge_way(ways[i]);
					tags[i] = KeyT(1);
				}
			}
		};

		template <typename KeyT, class T, int size = 8192, bool collect_stats = false>
		class Cache
		{
		public:
			using this_t = Cache<KeyT, T, size, collect_stats>;
			using entry_t = CacheEntry<KeyT, T>;
			using entry_purge_t = std::function<void(T&)>;

			Cache(entry_purge_t on_purge = [](T& t)
			{
				t = T();
			}) : hit_count(0), access_count(0), lru(0), mask(size-1), dirty(true), on_purge_(on_purge)
			{
				construct();
			}

			uint64_t get_access_count()
			{
				return access_count;
			}
			uint64_t get_hit_count()
			{
				return hit_count;
			}

			void construct()
			{
				pages_dirty.set();
				purge();

				hit_count = 0;
				access_count = 0;
			}
			void purge()
			{
				for(auto &i : cache) {
					i.purge(on_purge_);
				}

				pages_dirty.reset();
				dirty = false;
//
//				if(!dirty) return;
//				for(uint32_t i = 0; i < kCachePageCount; ++i) {
//					if(pages_dirty.test(i)) {
//						uint32_t page_start = i * kCachePageSize;
//						uint32_t page_end = (i+1) * kCachePageSize;
//						for(uint32_t entry = page_start; entry != page_end; ++entry) {
//							cache[entry].purge();
//						}
//					}
//				}
//
//				pages_dirty.reset();
//				dirty = false;
			}

			// A function to attempt a cache fetch and simultaneously update the cache
			inline CacheHitType try_cache_fetch(const KeyT _addr, T *&out)
			{
				size_t idx = std::hash<KeyT>()(_addr) & mask;
				if (collect_stats) access_count++;

				entry_t &entry = cache[idx];

				if (entry.tags[0] == _addr) {
					out = &entry.ways[0];
					__builtin_prefetch(out);

					if (collect_stats) hit_count++;

					return CACHE_HIT_WAY_0;
				} else if (entry.tags[1] == _addr) {
					out = &entry.ways[1];
					__builtin_prefetch(out);

					if (collect_stats) hit_count++;

					return CACHE_HIT_WAY_1;
				}
				out = &entry.ways[lru];
				entry.tags[lru] = _addr;
				lru = 1 - lru;

				dirty = true;
				pages_dirty.set(idx >> kCachePageBits);

				return CACHE_MISS;
			}

			inline CacheHitType is_cache_hit(KeyT _addr)
			{
				CacheHitType hit_type = CACHE_MISS;
				KeyT idx = (_addr) & mask;

				if (collect_stats) ++access_count;

				entry_t &entry = cache[idx];

				if (entry.tags[0] == _addr) {
					__builtin_prefetch(&entry.ways[0]);
					hit_type = CACHE_HIT_WAY_0;
					lru = CACHE_HIT_WAY_1;
					if (collect_stats) ++hit_count;
				} else if (entry.tags[1] == _addr) {
					__builtin_prefetch(&entry.ways[1]);
					hit_type = CACHE_HIT_WAY_1;
					lru = CACHE_HIT_WAY_0;
					if (collect_stats) ++hit_count;
				}
				return hit_type;
			}

			inline T *fetch_location(KeyT _addr, CacheHitType hit_type)
			{
				KeyT idx = (_addr) & mask;

				dirty = true;

				entry_t &entry = cache[idx];

				switch (hit_type) {
					case CACHE_HIT_WAY_0: {
						return &entry.ways[0];
					}
					case CACHE_HIT_WAY_1: {
						return &entry.ways[1];
					}

					case CACHE_MISS: {
						uint32_t victim = lru;

						// prefetch this since we'll be writing to it soon
						__builtin_prefetch(&entry.ways[victim]);

						lru = 1 - lru;
						entry.tags[victim] = _addr;
						return &entry.ways[victim];
					}

					default: {
						return 0;
					}
				}
			}

			inline void invalidate_location(KeyT _addr)
			{
				KeyT idx = (_addr) & mask;

				entry_t entry = cache[idx];
				entry.purge();
			}

		private:
			Cache(const this_t &other) = delete;

			CacheEntry<KeyT, T> cache[size];

			entry_purge_t on_purge_;

			uint64_t hit_count;
			uint64_t access_count;

			size_t mask;

			uint8_t lru;
			bool dirty;


			static const uint32_t kCachePageBits = 7;
			static const uint32_t kCachePageSize = 1 << kCachePageBits;
			static const uint32_t kCachePageCount = size/kCachePageSize;


			std::bitset<kCachePageCount> pages_dirty;

		};
	}
}
#endif /* _CACHE_H */
