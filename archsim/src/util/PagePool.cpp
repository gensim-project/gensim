/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PagePool.cpp
 *
 *  Created on: 6 Nov 2014
 *      Author: harry
 */

#include "util/PagePool.h"

#include <algorithm>
#include <cassert>
#include <sys/mman.h>

using namespace archsim::util;

//#define DEBUG
//#define DEBUG_TEXT

#ifdef DEBUG_TEXT
#define DEBUG_PRINT printf
#else
#define DEBUG_PRINT(x,...)
#endif

PageReference::PageReference(PageSet &parent, void *data, size_t num_pages) :
	Parent(parent), Data(data), Num_Pages(num_pages)
{
	assert(data);
}

PageReference::~PageReference()
{
	Parent.Return(Data, Num_Pages);
}

PageSet::PageSet(PagePool &parent) :
	Parent(parent)
{
	Start = mmap(NULL, kTotalSize, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);

	assert(Start && (Start != MAP_FAILED));
	FreePageSet set;
	set.NumPages = kNumPages;
	set.Start = Start;

	DEBUG_PRINT(" ** Creating initial free region %p %p\n", this, Start);

	free_regions.push_back(set);
}

PageSet::~PageSet()
{
	munmap(Start, kTotalSize);
}

bool PageSet::CanAllocate(long int NumPages, bool can_coalesce)
{
	assert(NumPages <= kNumPages);
	if (NumPages > kNumPages)
		return false;
	if (free_regions.empty())
		return false;

	for (auto i : free_regions) {
		if (i.NumPages >= NumPages)
			return true;
	}

	if (can_coalesce) {
		Coalesce();
		return CanAllocate(NumPages, false);
	}

	return false;
}

void *PageSet::Get(long int NumPages, bool can_coalesce)
{
	assert(NumPages <= kNumPages);
	assert(!free_regions.empty());

	DEBUG_PRINT(" ** Page set %p getting %lu\n", this, NumPages);

	for (auto region_i = free_regions.begin(); region_i != free_regions.end();
	     ++region_i) {
		auto &region = *region_i;
		if (region.NumPages >= NumPages) {
			void *data = region.Start;
			assert(data);

			DEBUG_PRINT(" ** Initial region free pointer %p\n", region.Start);

			region.Start = ((char*) region.Start) + (NumPages * kPageSize);
			region.NumPages -= NumPages;

			assert(region.Start);

			DEBUG_PRINT(" ** Region pointer updated to %p\n", region.Start);

			assert(region.NumPages >= 0 && region.NumPages < kNumPages);

			if (region.NumPages == 0) {
				DEBUG_PRINT(" ** Erasing free_region\n");
				free_regions.erase(region_i);
			}

//			fprintf(stderr, "Allocating pages %p (%lu)\n", data, NumPages);
			mprotect(data, NumPages * kPageSize, PROT_READ | PROT_WRITE);

#ifdef DEBUG
			for(char *d = (char*)data; d < (char*)data + NumPages * kPageSize; ++d) *d = 0xcd;
#endif

			return data;
		}
	}

	if (can_coalesce) {
		Coalesce();
		return Get(NumPages, false);
	}

	assert(!CanAllocate(NumPages, can_coalesce));

	return NULL;
}

bool PageSet::FreePageSetCmp(const PageSet::FreePageSet &a,
                             const PageSet::FreePageSet &b)
{
	return a.Start < b.Start;
}

void PageSet::Coalesce()
{
	DEBUG_PRINT(" ** Coalescing %p\n", this);
	bool changed = false;

	if (free_regions.size() < 2)
		return;

	DEBUG_PRINT(" ** ** Sorting free regions\n");
	std::sort(free_regions.begin(), free_regions.end(), FreePageSetCmp);

	do {
		changed = false;

		DEBUG_PRINT(" ** ** Coalesce iteration\n");

		auto end = free_regions.end() - 1;
		for (auto region_i = free_regions.begin(); region_i != end;
		     ++region_i) {
			auto &region_a = *region_i;
			auto &region_b = *(region_i + 1);

			assert(region_a.Start && region_b.Start);

			if (Coalesce(region_a, region_b)) {
				free_regions.erase(region_i + 1);
				changed = true;
				break;
			}
		}
	} while (changed);
}

bool PageSet::Coalesce(PageSet::FreePageSet &a, PageSet::FreePageSet &b)
{

	//If a and b are contiguous, a must be before b.
	//Therefore, the end of a must be the start of b
	void *a_end = (char*) a.Start + kPageSize * a.NumPages;

	if (a_end == b.Start) {
		a.NumPages += b.NumPages;

		DEBUG_PRINT(" ** ** Coalescing %p %p\n", a.Start, b.Start);

		assert(a.NumPages <= kNumPages);

		return true;
	}

	return false;
}

void PageSet::Return(void *ptr, size_t NumPages)
{
	assert(ptr);
	assert(NumPages);

	FreePageSet new_free_region;
	new_free_region.NumPages = NumPages;
	new_free_region.Start = ptr;

	DEBUG_PRINT(" ** Adding free region %p %p\n", this, ptr);
	free_regions.push_back(new_free_region);

#ifdef DEBUG
	mprotect(ptr, NumPages * kPageSize, PROT_WRITE);
	for(char *d = (char*)ptr; d < (char*)ptr +NumPages*kPageSize; ++d) *d = 0xab;
#endif
//	fprintf(stderr, "Returning pages %p (%lu)\n", ptr, NumPages);
	mprotect(ptr, NumPages * kPageSize, PROT_NONE);
}

PagePool::PagePool() :
	curr_set(NULL)
{
}

PageReference *PagePool::Allocate(size_t num_pages)
{
	std::lock_guard<std::mutex> l(lock);
	assert(num_pages);
	assert(num_pages < PageSet::kNumPages);
	if (!curr_set || !curr_set->CanAllocate(num_pages, false)) {

		curr_set = new PageSet(*this);
		page_sets.insert(curr_set);

		DEBUG_PRINT(" ** New Page set %p\n", curr_set);
	}

	void *data = curr_set->Get(num_pages, false);
	assert(data);
	return new PageReference(*curr_set, data, num_pages);
}

void PagePool::GC()
{
	std::lock_guard<std::mutex> l(lock);
	DEBUG_PRINT(" ** GC\n");
	bool changed = false;
	do {
		changed = false;
		DEBUG_PRINT(" ** GC Iteration\n");

		for (auto i = page_sets.begin(); i != page_sets.end(); ++i) {
			PageSet *page_set = *i;

			DEBUG_PRINT(" ** GC Inspecting %p\n", page_set);
			page_set->Coalesce();

			if (page_set->Empty()) {
				DEBUG_PRINT(" ** %p deleting page set %p\n", this, page_set);
				delete page_set;
				page_sets.erase(i);

				if(page_set == curr_set) curr_set = NULL;

				changed = true;
				break;
			}
		}
	} while (changed);
}
