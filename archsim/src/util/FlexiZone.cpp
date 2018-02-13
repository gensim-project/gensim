/*
 * FlexiZone.cpp
 *
 *  Created on: 6 Nov 2014
 *      Author: harry
 */

#include "util/FlexiZone.h"

#include <cassert>
#include <cstdio>

using namespace archsim::util;

FlexiZoneChunk::FlexiZoneChunk(size_t size, uint32_t flags) : chunk_size(size)
{
	assert((size & (kPageSize-1)) == 0);
	data = (char*)mmap(NULL, size, flags, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(data);
	data_ptr = data;

	this->chunk_size += kPageSize-1;
	this->chunk_size &= ~(kPageSize-1);
}

FlexiZoneChunk::~FlexiZoneChunk()
{
	munmap(data, chunk_size);
}

void *FlexiZoneChunk::Allocate(size_t size, uint32_t alignment)
{
	if(size > chunk_size) return NULL;
	char *orig_data = data_ptr;

	data_ptr += alignment-1;
	data_ptr = (char*)((size_t)data_ptr & ~(alignment-1));

	uint32_t align_bytes = data_ptr - orig_data;

	data_ptr += size;

	return orig_data + align_bytes;
}

bool FlexiZoneChunk::HasCapacity(uint32_t size, uint32_t alignment)
{
	if(size > chunk_size) return false;
	char *test_data = data_ptr;

	test_data += alignment-1;
	test_data = (char*)((size_t)test_data & ~(alignment-1));

	test_data += size;
	return test_data < (data + this->chunk_size);
}

bool FlexiZoneChunk::SetProtFlags(uint32_t new_flags)
{
	return !mprotect(data, chunk_size, new_flags);
}

FlexiZone::FlexiZone() : curr_chunk(NULL)
{

}

FlexiZone::~FlexiZone()
{
	Clear();
}

void *FlexiZone::Allocate(size_t size, uint32_t alignment)
{
	if(!alignment) alignment = 16;
	if(!curr_chunk || !curr_chunk->HasCapacity(size, alignment)) {
		size_t needed_pages = 1 + (size / FlexiZoneChunk::kPageSize);

		curr_chunk = new FlexiZoneChunk(needed_pages * FlexiZoneChunk::kPageSize, kDefaultFlags);
		chunks.push_back(curr_chunk);
	}

	void* z = curr_chunk->Allocate(size, alignment);
	return z;
}

void FlexiZone::Clear()
{
	for(auto i : chunks) delete i;
}

bool FlexiZone::SetProtFlags(uint32_t new_flags)
{
	bool success = true;
	for(auto i : chunks) success &= i->SetProtFlags(new_flags);
	return success;
}

