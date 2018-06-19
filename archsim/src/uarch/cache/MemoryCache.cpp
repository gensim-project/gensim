/*
 * uarch/cache/MemoryCache.cpp
 */
#include "uarch/cache/MemoryCache.h"

#include <iomanip>

using namespace archsim::uarch::cache;
using namespace archsim::uarch::cache::types;

MemoryCache::MemoryCache(std::string name) : name(name), last_block(0)
{
}

MemoryCache::~MemoryCache()
{

}

bool MemoryCache::Access(archsim::core::thread::ThreadInstance *cpu, CacheAccessType accessType, Address phys_addr, Address virt_addr, uint8_t size)
{
	/*if (accessType == MemoryCache::InstructionFetch) {
		uint32_t block = phys_addr / 16;
		//fprintf(stderr, "%u %u %08x\n", block, last_block, phys_addr);
		if (last_block == block) {
			return true;
		} else {
			last_block = block;
		}
	}*/

	if (IsHit(accessType, phys_addr, virt_addr, size)) {
		hits.inc();
		return true;
	} else {
		misses.inc();

		MemoryCache *next = GetNext(accessType);
		if (next) {
			return next->Access(cpu, accessType, phys_addr, virt_addr, size);
		} else {
			return true;
		}
	}
}

void MemoryCache::PrintStatistics(std::ostream& stream)
{
	stream << std::left << std::setw(15) << name;
	stream << std::right << std::dec << std::fixed << std::setw(12) << hits.get_value();
	stream << std::right << std::dec << std::fixed << std::setw(12) << misses.get_value();
	stream << std::right << std::dec << std::fixed << std::setw(12) << (hits.get_value() + misses.get_value()) << std::endl;
}

bool MemoryCache::IsHit(CacheAccessType accessType, Address phys_addr, Address virt_addr, uint8_t size)
{
	return true;
}

void MemoryCache::Flush()
{
}

MainMemory::MainMemory() : MemoryCache("MM")
{

}

MainMemory::~MainMemory()
{
}

bool MainMemory::IsHit(CacheAccessType accessType, Address phys_addr, Address virt_addr, uint8_t size)
{
	return true;
}

Associative::Associative(std::string name, uint32_t size, uint8_t ways, uint8_t linesize, ReplacementPolicy rp, AddressType indexType, AddressType tagType)
	: MemoryCache(name), size(size), ways(ways), linesize(linesize), rp(rp), indexType(indexType), tagType(tagType), mru(0)
{
	entries = (size / linesize);
	entries_per_way = (entries / ways);

	cache = (cache_entry *)calloc(entries, sizeof(cache_entry));
	way_ages = (uint8_t *)calloc(ways, sizeof(*way_ages));

	for (int i = 0; i < ways; i++) {
		way_ages[i] = i;
	}
}

Associative::~Associative()
{
	free(way_ages);
	free(cache);
}

#define CE_FLAG_VALID (1 << 0)
#define CE_FLAG_DIRTY (1 << 1)

bool Associative::IsHit(CacheAccessType accessType, Address phys_addr, Address virt_addr, uint8_t size)
{
	uint32_t index = 0, tag = 0;
	bool sequential = phys_addr == last_phys_addr + 4;
	last_phys_addr = phys_addr;
	last_virt_addr = virt_addr;

	switch(indexType) {
		case Virtual:
			index = (virt_addr.Get() / linesize) % entries_per_way;
			break;
		case Physical:
			index = (phys_addr.Get() / linesize) % entries_per_way;
			break;
	}

	switch(tagType) {
		case Virtual:
			tag = (virt_addr.Get() / linesize) / entries_per_way;
			break;
		case Physical:
			tag = (phys_addr.Get() / linesize) / entries_per_way;
			break;
	}

	for (int i = 0; i < ways; i++) {
		const cache_entry *entry = &cache[index + (entries_per_way * i)];
		if ((entry->tag == tag) && (entry->flags & CE_FLAG_VALID)) {
			mru = i;
			return true;
		}
	}

	uint32_t victim = 0;
	switch (rp) {
		case LRU:
			for (int i = 0; i < ways; i++) {
				if (way_ages[i] == 0) {
					way_ages[i] = ways-1;
					victim = index + (entries_per_way * i);
				} else {
					way_ages[i]--;
				}
			}

			break;

		case MRU:
			victim = index + (entries_per_way * (!mru % ways));
			mru = !mru % ways;
			break;

		case Random:
			victim = index + (entries_per_way * (random() % ways));
			break;
	}

	// Update the cache
	cache[victim].tag = tag;
	cache[victim].flags |= CE_FLAG_VALID;

	return false;
}

void Associative::Flush()
{
	for (uint32_t way = 0; way < ways; way++) {
		for (uint32_t line = 0; line < entries_per_way; line++) {
			cache[line * way].flags &= ~CE_FLAG_VALID;
		}
	}
}

void Associative::InvalidateLine(uint8_t way, uint32_t line)
{
	assert(line < entries_per_way);
	assert(way < ways);

	cache[line * way].flags &= ~CE_FLAG_VALID;
}

void Associative::InvalidateWay(uint8_t way)
{
	assert(way < ways);

	for (uint32_t line = 0; line < entries_per_way; line++) {
		cache[line * way].flags &= ~CE_FLAG_VALID;
	}
}
