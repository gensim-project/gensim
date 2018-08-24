/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * TranslationCache.cpp
 *
 *  Created on: 22 Jul 2015
 *      Author: harry
 */

#include "define.h"
#include "translate/TranslationCache.h"
#include "translate/profile/RegionArch.h"

#include <cstring>

using namespace archsim::translate;

TranslationCache::TranslationCache()
{
	InvalidateAll();
}

void TranslationCache::InvalidateAll()
{
	bzero(region_txln_cache, sizeof(region_txln_cache));
	dirty_pages.reset();
}

void TranslationCache::Invalidate()
{
	if(dirty_pages.none()) return;

	// Clear only the pages which are marked dirty
	for(uint32_t i = 0; i < dirty_pages.size(); ++i) {
		if(dirty_pages.test(i)) {
			char *ptr = (char*)region_txln_cache;
			ptr += cache_page_size * i;
			bzero(ptr, cache_page_size);
			dirty_pages.reset(i);
		}
	}

	assert(dirty_pages.none());
}

void TranslationCache::InvalidateEntry(Address virt_addr)
{
	if(dirty_pages.none())return;
	uint32_t page_index = virt_addr.GetPageIndex();
	region_txln_cache[page_index] = 0;
}
