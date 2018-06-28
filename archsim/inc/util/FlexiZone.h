/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * FlexiZone.h
 *
 *  Created on: 6 Nov 2014
 *      Author: harry
 */

#ifndef FLEXIZONE_H_
#define FLEXIZONE_H_

#include <cstdint>
#include <cstddef>

#include <sys/mman.h>

#include <vector>

namespace archsim
{
	namespace util
	{

		class FlexiZone;

		class FlexiZoneChunk
		{
			friend class FlexiZone;
		private:
			FlexiZoneChunk(size_t size, uint32_t flags);
			~FlexiZoneChunk();

			void *Allocate(size_t size, uint32_t alignment);
			bool HasCapacity(uint32_t size, uint32_t aligment);

			bool SetProtFlags(uint32_t new_flags);

			static const size_t kPageSize = 4096;
			size_t chunk_size;
			char *data;
			char *data_ptr;
		};

		class FlexiZone
		{
		public:
			FlexiZone();
			~FlexiZone();

			const uint32_t kDefaultFlags = PROT_READ | PROT_WRITE;

			void* Allocate(size_t size, uint32_t alignment=16);
			void Clear();
			bool SetProtFlags(uint32_t new_flags);

		private:
			FlexiZoneChunk *curr_chunk;
			std::vector<FlexiZoneChunk*> chunks;
		};

	}
}


#endif /* FLEXIZONE_H_ */
