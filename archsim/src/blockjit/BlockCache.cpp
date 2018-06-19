/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "blockjit/BlockCache.h"

using namespace archsim::blockjit;

void BlockCache::Invalidate()
{
	memset(cache_.data(), 0xff, sizeof(BlockCacheEntry) * kCacheSize);
}
