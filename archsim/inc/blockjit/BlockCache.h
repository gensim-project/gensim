/*
 * BlockCache.h
 *
 *  Created on: 30 Nov 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCKCACHE_H_
#define INC_BLOCKJIT_BLOCKCACHE_H_

#include "blockjit/ir.h"
#include "abi/Address.h"
#include "blockjit/blockcache-defines.h"
#include "core/thread/ProcessorFeatures.h"

#include <array>
#include <cstring>

namespace archsim
{
	using captive::shared::block_txln_fn;

	namespace blockjit
	{

		struct BlockCacheEntry {
			uint64_t virt_tag;
			block_txln_fn ptr;
		};

// Need BlockCacheEntry to be 16 bytes to be compatible with block trampoline assembly and dispatch logic
		static_assert(sizeof(struct BlockCacheEntry) == 16, "Block Cache Entry must be 16 bytes!");

// don't care how big the features set is though
		class BlockFeaturesEntry
		{
		public:
			BlockFeaturesEntry() : feature_required_mask(0), feature_level(0) {}

			uint64_t feature_required_mask;
			uint64_t feature_level;
		};

		class BlockCache
		{
		public:
			BlockCache()
			{
				Invalidate();
			}

			bool Contains(Address address) const
			{
				const auto &entry = GetEntry(address);
				return entry.virt_tag == address.Get();
			}

			block_txln_fn Lookup(Address address) const
			{
				const auto &entry = GetEntry(address);
				if(entry.virt_tag == address.Get()) return entry.ptr;
				return NULL;
			}

			void Insert(Address address, block_txln_fn ptr, const archsim::ProcessorFeatureSet &required_features)
			{
				auto &entry = GetEntry(address);
				entry.virt_tag = address.Get();
				entry.ptr = ptr;

				auto &features = GetFeatures(address);
				features.feature_required_mask = required_features.GetRequiredMask();
				features.feature_level = required_features.GetLevelMask();
			}

			void Invalidate();

			void InvalidateFeatures(uint64_t current_mask)
			{
				for(unsigned i = 0; i < kCacheSize; ++i) {
					const auto &features = feature_cache_[i];
					__builtin_prefetch((void*)&(cache_[i+1]));
					__builtin_prefetch((void*)&(feature_cache_[i+1]));
					if((current_mask & features.feature_required_mask) != features.feature_level) {
						cache_[i].virt_tag = 1;
					}
				}
			}

			struct BlockCacheEntry *GetPtr()
			{
				return cache_.data();
			}

			static const uint32_t kCacheBits = BLOCKCACHE_SIZE_BITS;
			static const uint32_t kCacheSize = BLOCKCACHE_SIZE;

			// XXX ARM HAX
			static const uint32_t kInstructionShift = BLOCKCACHE_INSTRUCTION_SHIFT;

		

			const BlockCacheEntry &GetEntry(Address address) const
			{
				return cache_[(address.Get() >> kInstructionShift) % kCacheSize];
			}
			BlockCacheEntry &GetEntry(Address address)
			{
				return cache_[(address.Get() >> kInstructionShift) % kCacheSize];
			}
		private:
			const BlockFeaturesEntry &GetFeatures(Address address) const
			{
				return feature_cache_[(address.Get() >> kInstructionShift) % kCacheSize];
			}
			BlockFeaturesEntry &GetFeatures(Address address)
			{
				return feature_cache_[(address.Get() >> kInstructionShift) % kCacheSize];
			}

			// We really want the BlockCacheEntry struct to be 16 bytes

			std::array<BlockCacheEntry, kCacheSize> cache_;
			std::array<BlockFeaturesEntry, kCacheSize> feature_cache_;
		};

	}
}



#endif /* INC_BLOCKJIT_BLOCKCACHE_H_ */
