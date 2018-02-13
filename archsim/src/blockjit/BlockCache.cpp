
#include "blockjit/BlockCache.h"

using namespace archsim::blockjit;

void BlockCache::Invalidate()
{
	memset(cache_.data(), 0xff, sizeof(BlockCacheEntry) * kCacheSize);
}