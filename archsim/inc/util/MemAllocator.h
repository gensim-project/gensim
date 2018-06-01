/*
 * MemAllocator.h
 *
 *  Created on: 24 Sep 2015
 *      Author: harry
 */

#ifndef INC_UTIL_MEMALLOCATOR_H_
#define INC_UTIL_MEMALLOCATOR_H_

#include <atomic>
#include <sys/mman.h>
#include <cassert>
#include <cstring>
#include <list>
#include <map>
#include <set>

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
#include <valgrind/memcheck.h>7
#endif
#endif

//#define DEBUG_MEMALLOCATOR

namespace wulib
{
	class MemAllocator
	{
	public:
		virtual ~MemAllocator();

		virtual void *Allocate(size_t size_bytes) = 0;
		virtual void *Reallocate(void *buffer, size_t new_size_bytes) = 0;
		virtual void Free(void *buffer) = 0;
	};

	/*
	 * A simple allocator which just forwards to malloc/realloc/free.
	 */
	class StandardMemAllocator : public MemAllocator
	{
	public:
		virtual ~StandardMemAllocator();

		void *Allocate(size_t size_bytes) override;
		void *Reallocate(void *buffer, size_t new_size_bytes) override;
		void Free(void *buffer) override;
	};

	/*
	 * A high performance allocator which allocates in larger chunks, but can
	 * allocate out of these chunks very efficiently.
	 */
	class SimpleZoneMemAllocator : public MemAllocator
	{

		class Chunk;
		struct AllocationHeader {
			Chunk *parent;
			size_t size;
		};

		class Chunk
		{
		public:
			Chunk() : _allocated_pointers(0)
			{
				_data = (uint8_t*)mmap(NULL, kChunkSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
				_data_ptr = _data;

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
				VALGRIND_CREATE_MEMPOOL(_data, 0, 0);
#endif
#endif
			}
			~Chunk()
			{
#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
				VALGRIND_DESTROY_MEMPOOL(_data);
#endif
#endif
				munmap(_data, kChunkSize);
			}

			size_t ConsumedSpace() const
			{
				return _data_ptr - _data;
			}
			size_t RemainingSpace() const
			{
				return kChunkSize - ConsumedSpace();
			}

			void *Data()
			{
				return _data;
			}

			void *Allocate(size_t bytes)
			{
				assert(bytes < kChunkSize);

				// Make sure we can actually fit the new contents in
				if((bytes+sizeof(AllocationHeader)) > RemainingSpace()) return NULL;

				// Create the chunk header and fill it in
				AllocationHeader *hdr = (AllocationHeader*)_data_ptr;

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
				VALGRIND_MEMPOOL_ALLOC(_data, hdr, sizeof(*hdr));
#endif
#endif

				Extend(sizeof(*hdr));
				hdr->parent = this;
				hdr->size = bytes;

				// Actually allocate space for the data
				void *curr_ptr = (void*)_data_ptr;

				Extend(bytes);
				Align();

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
				VALGRIND_MEMPOOL_ALLOC(_data, curr_ptr, bytes);
#endif
#endif

				// Record that we have allocated another pointer and return it
				_allocated_pointers++;

				// Curr pointer must be aligned to 16 bytes
				assert(((uint64_t)curr_ptr & 0xf) == 0);
				return curr_ptr;
			}

			void Align()
			{
				if((uint64_t)_data_ptr & 0xf) {
					_data_ptr += 0x10;
					_data_ptr = (uint8_t*)((uint64_t)_data_ptr & ~0xf);
				}
			}

			void Extend(size_t bytes)
			{
				assert(RemainingSpace() >= bytes);
				_data_ptr += bytes;
				Align();
			}

			void Shrink(size_t bytes)
			{
				_data_ptr -= bytes;
				assert(_data_ptr >= _data);
				Align();
			}

			void Free(void *ptr)
			{
#ifndef NDEBUG
				uint8_t *cptr = (uint8_t*)ptr;
				assert(cptr >= _data && cptr < _data+kChunkSize);
#endif
				_allocated_pointers--;

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
				VALGRIND_MEMPOOL_FREE(_data, ptr);

				auto header_ptr = (char*)ptr - sizeof(AllocationHeader);
				VALGRIND_MEMPOOL_FREE(_data, header_ptr);
#endif
#endif

			}

			bool Empty() const
			{
				return _allocated_pointers == 0;
			}

			static const size_t kChunkSize = 1024*1024;
		private:
			uint32_t _allocated_pointers;
			uint8_t *_data_ptr;
			uint8_t *_data;
		};

	public:
		SimpleZoneMemAllocator();
		virtual ~SimpleZoneMemAllocator();

		void *Allocate(size_t size_bytes) override;
		void *Reallocate(void *buffer, size_t new_size_bytes) override;
		void Free(void *buffer) override;

		float Efficiency() const;
	private:
		typedef Chunk chunk_t;
		std::list<chunk_t*> _chunks;
		chunk_t *_active_chunk;
		void *_last_buffer;

#ifdef DEBUG_MEMALLOCATOR
		uint64_t _allocated_bytes;
#endif
	};

}

#endif /* INC_UTIL_MEMALLOCATOR_H_ */
