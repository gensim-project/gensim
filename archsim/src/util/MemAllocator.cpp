/*
 * MemAllocator.cpp
 *
 *  Created on: 24 Sep 2015
 *      Author: harry
 */

#include "util/MemAllocator.h"
#include <malloc.h>

using namespace wulib;

MemAllocator::~MemAllocator()
{

}

StandardMemAllocator::~StandardMemAllocator()
{

}

void *StandardMemAllocator::Allocate(size_t size_bytes)
{
	auto ptr = malloc(size_bytes);
	auto bytes = ((size_bytes + 4096) / 4096) * 4096;
	auto result = mprotect((void*)((intptr_t)(ptr) & ~0xfffULL), bytes, PROT_READ|PROT_WRITE|PROT_EXEC);
	
	if(result != 0) {
		throw std::logic_error("Invalid mprotect: " + std::string(strerror(errno)));
	}
	
	return ptr;
}

void *StandardMemAllocator::Reallocate(void *buffer, size_t new_size_bytes)
{
	auto ptr = realloc(buffer, new_size_bytes);
	auto bytes = ((new_size_bytes + 4096) / 4096) * 4096;
	auto result = mprotect((void*)((intptr_t)(ptr) & ~0xfffULL), bytes, PROT_READ|PROT_WRITE|PROT_EXEC);
	
	if(result != 0) {
		throw std::logic_error("Invalid mprotect: " + std::string(strerror(errno)));
	}
	
	return ptr;
}

void StandardMemAllocator::Free(void *buffer)
{
	free(buffer);
}

SimpleZoneMemAllocator::SimpleZoneMemAllocator() : _active_chunk(NULL), _last_buffer(NULL)
{

}

SimpleZoneMemAllocator::~SimpleZoneMemAllocator()
{
//	assert(_active_chunk == NULL);
#ifdef DEBUG_MEMALLOCATOR
	printf("Allocator efficiency: %f\n", Efficiency());
	printf("Bytes allocated at exit: %lu\n", (long unsigned int)_allocated_bytes);
#endif
}

void *SimpleZoneMemAllocator::Allocate(size_t size_bytes)
{
#ifdef DEBUG_MEMALLOCATOR
	_allocated_bytes += size_bytes;
#endif
	if(!_active_chunk || _active_chunk->RemainingSpace() < size_bytes) {
		_active_chunk = new chunk_t();
		_chunks.push_back(_active_chunk);
	}

	void *allocation = _active_chunk->Allocate(size_bytes);
	if(!allocation) {
		_active_chunk = new chunk_t();
		_chunks.push_back(_active_chunk);
		allocation = _active_chunk->Allocate(size_bytes);
	}

	_last_buffer = allocation;
	return allocation;
}

void *SimpleZoneMemAllocator::Reallocate(void *buffer, size_t new_size_bytes)
{
	AllocationHeader *hdr = (AllocationHeader*)(((uint8_t*)buffer)-sizeof(AllocationHeader));

	// If we just allocated this buffer, we might be able to extend it straight away
	// without actually reallocating

	if((buffer == _last_buffer) && (buffer != NULL)) {
		if(new_size_bytes > hdr->size) {
			size_t extra_space = (new_size_bytes - hdr->size);

			if(hdr->parent->RemainingSpace() >= extra_space) {

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
				VALGRIND_MEMPOOL_FREE(hdr->parent->Data(), buffer);
				VALGRIND_MEMPOOL_ALLOC(hdr->parent->Data(), buffer, new_size_bytes);
				VALGRIND_MAKE_MEM_DEFINED(buffer, hdr->size);
#endif
#endif

#ifdef DEBUG_MEMALLOCATOR
				_allocated_bytes += extra_space;
#endif

				hdr->size = new_size_bytes;
				hdr->parent->Extend(extra_space);
				return buffer;
			}
		} else {
			size_t reduce_by = hdr->size - new_size_bytes;

#ifndef NDEBUG
#ifdef CONFIG_VALGRIND
			VALGRIND_MEMPOOL_FREE(hdr->parent->Data(), buffer);
			VALGRIND_MEMPOOL_ALLOC(hdr->parent->Data(), buffer, new_size_bytes);
			VALGRIND_MAKE_MEM_DEFINED(buffer, new_size_bytes);
#endif
#endif

#ifdef DEBUG_MEMALLOCATOR
			_allocated_bytes -= reduce_by;
#endif

			hdr->size = new_size_bytes;
			hdr->parent->Shrink(reduce_by);
			return buffer;
		}
	}

	void *new_buffer = Allocate(new_size_bytes);

	if(buffer) {
		size_t copy_size;
		if(new_size_bytes > hdr->size) copy_size = hdr->size;
		else copy_size = new_size_bytes;

		memcpy(new_buffer, buffer, copy_size);

		Free(buffer);
	}

	return new_buffer;
}

void SimpleZoneMemAllocator::Free(void *buffer)
{
	AllocationHeader *hdr = (AllocationHeader*)(((uint8_t*)buffer)-sizeof(AllocationHeader));

#ifdef DEBUG_MEMALLOCATOR
	_allocated_bytes -= hdr->size;
#endif

	hdr->parent->Free(buffer);
	if(hdr->parent->Empty()) {
		_chunks.remove(hdr->parent);
		if(hdr->parent == _active_chunk) _active_chunk = NULL;
		delete hdr->parent;
	}

	if(buffer == _last_buffer) _last_buffer = NULL;
}

float SimpleZoneMemAllocator::Efficiency() const
{
	size_t total_space = 0;
	size_t total_consumed = 0;
	for(auto *chunk : _chunks) {
		total_space += Chunk::kChunkSize;
		total_consumed += chunk->ConsumedSpace();
	}
	return (float)total_consumed / total_space;
}
