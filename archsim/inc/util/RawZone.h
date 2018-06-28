/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * RawZone.h
 *
 *  Created on: 19 Aug 2014
 *      Author: harry
 */

#ifndef RAWZONE_H_
#define RAWZONE_H_

#include <list>
#include <vector>
#include <sys/mman.h>

namespace archsim
{
	namespace util
	{

		template <uint32_t buffer_size, uint32_t ChunkSize, uint32_t Align=8> class RawZoneChunk
		{
		public:
			typedef uint8_t return_t[buffer_size];

			RawZoneChunk()
			{
				data_ptr = data;
			}
			~RawZoneChunk()
			{
				munmap(data, ChunkSize);
			}

			bool Full()
			{
				return (data_ptr + buffer_size) >= (((uint8_t*)data) + ChunkSize);
			}

			void Clear()
			{
				data_ptr = data;
			}

			return_t *Construct()
			{
				return_t* t = (return_t*)data_ptr;
				data_ptr += buffer_size;
				return t;
			}
		private:
			uint8_t data[ChunkSize];
			uint8_t *data_ptr;
		};

		/**
		 * RawZone: a zone allocator managing blocks of raw memory
		 */
		template <uint32_t buffer_size, uint32_t ChunkSize, uint32_t Align=8> class RawZone
		{
		public:
			typedef uint8_t return_t[buffer_size];

			RawZone() : chunk(0) {}
			~RawZone()
			{
				for(auto *c : chunks) munmap(c, sizeof(*c));
				for(auto *c : empty_chunks) munmap(c, sizeof(*c));
			}

			return_t* Construct()
			{
				if(!chunk || chunk->Full()) chunk = NewChunk();
				assert(!chunk->Full());
				assert(chunk);
				return chunk->Construct();
			}

			void Clear()
			{
				for(auto *c : chunks) {
					c->Clear();
					empty_chunks.push_back(c);
				}
				chunks.clear();
				chunk = 0;
			}

		private:
			typedef RawZoneChunk<buffer_size, ChunkSize, Align> chunk_t;

			chunk_t *NewChunk()
			{
				if(!empty_chunks.empty()) {
					chunk_t *newchunk = empty_chunks.back();
					empty_chunks.pop_back();
					chunks.push_back(newchunk);
					return newchunk;
				} else {
					chunk_t *newchunk = new (mmap(NULL, sizeof(chunk_t), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0))chunk_t();
					chunks.push_back(newchunk);
					return newchunk;
				}
			}

			chunk_t *chunk;
			std::vector<chunk_t*> chunks;
			std::list<chunk_t*> empty_chunks;
		};
	}
}


#endif /* RAWZONE_H_ */
