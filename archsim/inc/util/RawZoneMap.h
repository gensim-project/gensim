/*
 * RawZoneMap.h
 *
 *  Created on: 28 Aug 2014
 *      Author: harry
 */

#ifndef RAWZONEMAP_H_
#define RAWZONEMAP_H_

#include "util/RawZone.h"

#include <unordered_map>

namespace archsim
{
	namespace util
	{

		template<typename key_type, uint32_t block_size, uint32_t chunk_size> class RawZoneMap
		{
		private:
			typedef archsim::util::RawZone<block_size, chunk_size> zone_t;
		public:
			typedef typename zone_t::return_t block_t;

		private:
			zone_t zone;
			std::unordered_map<key_type, block_t*> allocated_blocks;

		public:
			static const uint32_t BlockSize = block_size;

			block_t *Get(key_type key)
			{
				auto &rval = allocated_blocks[key];
				if(!rval) rval = zone.Construct();
				return rval;
			}

			size_t Count()
			{
				return allocated_blocks.size();
			}

			void Clear()
			{
				zone.Clear();
				allocated_blocks.clear();
			}
		};


	}
}


#endif /* RAWZONEMAP_H_ */
