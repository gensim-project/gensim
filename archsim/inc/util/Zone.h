/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * Zone.h
 *
 *  Created on: 13 Aug 2014
 *      Author: harry
 */

#ifndef ZONE_H_
#define ZONE_H_

#include <vector>

namespace archsim
{
	namespace util
	{

		template <typename T, uint32_t ChunkSize, uint32_t Align=8> class ZoneChunk
		{
		public:
			ZoneChunk() : data_ptr(data) {}
			~ZoneChunk()
			{
				T* d = (T*)data;
				while(d < (T*)data_ptr) {
					d->~T();
					d++;
				}
			}

			bool Full()
			{
				return (data_ptr + sizeof(T)) >= ((uint8_t*)data + sizeof(data));
			}

			template<typename... args> T* Construct(args&&... A)
			{
				T* t = new (data_ptr) T(A...);
				data_ptr += sizeof(T);
				return t;
			}
		private:
			uint8_t data[ChunkSize];
			uint8_t *data_ptr;
		};

		/**
		 * Zone: A zone allocator designed for fast allocation of contiguous blocks of many small homogeneous objects
		 */
		template <typename T, uint32_t ChunkSize, uint32_t Align=8> class Zone
		{
		public:
			Zone() : chunk(0) {}
			~Zone()
			{
				for(auto c : chunks) {
					delete c;
				}
			}

			template<typename... args> T* Construct(args&&... A)
			{
				if(!chunk || chunk->Full()) chunk = NewChunk();
				return chunk->Construct(A...);
			}

			size_t GetAllocatedSize() const
			{
				return ChunkSize * chunks.size();
			}

		private:
			typedef ZoneChunk<T, ChunkSize, Align> chunk_t;

			chunk_t *NewChunk()
			{
				chunk_t *newchunk = new chunk_t();
				chunks.push_back(newchunk);
				return newchunk;
			}

			chunk_t *chunk;
			std::vector<chunk_t*> chunks;
		};
	}
}


#endif /* ZONE_H_ */
