/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * BlockCache.h
 *
 *  Created on: 19 Aug 2015
 *      Author: harry
 */

#ifndef INC_ABI_DEVICES_GENERIC_BLOCK_BLOCKCACHE_H_
#define INC_ABI_DEVICES_GENERIC_BLOCK_BLOCKCACHE_H_

#include <cstring>

namespace archsim
{

	namespace abi
	{

		namespace devices
		{

			namespace generic
			{

				namespace block
				{

					class BlockCache
					{

					public:
						static const uint32_t kBlockSize = 512;
						static const uint32_t kCacheNumEntries = (32 * (1 << 20)) / kBlockSize;
						static const size_t kInvalidIdx = -1;

						bool HasBlock(size_t block_idx) const
						{
							return block_idxs[block_idx % kCacheNumEntries] == block_idx;
						}

						bool ReadBlock(size_t block_idx, uint8_t *target) const
						{
							memcpy(target, block_data[block_idx % kCacheNumEntries], kBlockSize);
							return true;
						}

						bool WriteBlock(size_t block_idx, const uint8_t *data)
						{
							memcpy(block_data[block_idx % kCacheNumEntries], data, kBlockSize);
							block_idxs[block_idx % kCacheNumEntries] = block_idx;
							return true;
						}

						BlockCache()
						{
							block_idxs = new size_t[kCacheNumEntries];
							block_data = new block_t[kCacheNumEntries];

							for(uint64_t i = 0; i < kCacheNumEntries; ++i) {
								block_idxs[i] = kInvalidIdx;
							}
						}

						~BlockCache()
						{
							delete [] block_idxs;
							delete [] block_data;
						}

					private:
						typedef uint8_t block_t[kBlockSize];

						size_t *block_idxs;
						block_t *block_data;


					};

				}  // namespace block

			}  // namespace generic

		}  // namespace devices

	}  // namespace abi

}  // namespace archsim

#endif /* INC_ABI_DEVICES_GENERIC_BLOCK_BLOCKCACHE_H_ */
