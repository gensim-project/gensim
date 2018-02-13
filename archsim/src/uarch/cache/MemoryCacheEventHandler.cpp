#include "uarch/cache/MemoryCacheEventHandler.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"

#include <unordered_set>

using namespace archsim::uarch::cache;

MemoryCacheEventHandler::MemoryCacheEventHandler() : MemoryCache("")
{

}


bool MemoryCacheEventHandler::HandleEvent(gensim::Processor& cpu, archsim::abi::memory::MemoryModel::MemoryEventType type, archsim::abi::memory::guest_addr_t addr, uint8_t size)
{
	virt_addr_t virt = (phys_addr_t)addr;
	/*abi::memory::SystemMemoryModel& smm = (abi::memory::SystemMemoryModel&)cpu.GetMemoryModel();*/

	phys_addr_t phys = virt;
	/*if (&smm == NULL || &smm.GetMMU() == NULL) {
		phys = virt;
	} else {
		if (smm.GetMMU().Translate(&cpu, virt, phys, cpu.build_mmu_access_word(type == archsim::abi::memory::MemoryModel::MemEventWrite), false) != archsim::abi::devices::MMU::TXLN_OK)
			return true;
	}*/

	switch (type) {
		case archsim::abi::memory::MemoryModel::MemEventRead:
			return Access(cpu, MemoryCache::DataRead, phys, virt, size);
		case archsim::abi::memory::MemoryModel::MemEventWrite:
			return Access(cpu, MemoryCache::DataWrite, phys, virt, size);
		case archsim::abi::memory::MemoryModel::MemEventFetch:
			return Access(cpu, MemoryCache::InstructionFetch, phys, virt, size);
	}

	return false;
}

void MemoryCacheEventHandler::PrintGlobalCacheStatistics(std::ostream& stream)
{
	std::unordered_set<MemoryCache *> seen;

	stream << "Cache Statistics" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;
	stream << "Name           Hits        Misses      Total" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;
	PrintStatisticsRecursive(stream, seen, *this);
	stream << "-----------------------------------------------------" << std::endl;
}

void MemoryCacheEventHandler::PrintStatisticsRecursive(std::ostream& stream, std::unordered_set<MemoryCache *>& seen, MemoryCache& cache)
{
	std::unordered_set<MemoryCache *> display;

	for (auto next : cache.nextCaches) {
		if (seen.count(next.second) == 0) {
			seen.insert(next.second);
			display.insert(next.second);

			next.second->PrintStatistics(stream);
		}
	}

	for (auto disp : display) {
		PrintStatisticsRecursive(stream, seen, *disp);
	}
}

bool MemoryCacheEventHandler::IsHit(CacheAccessType accessType, phys_addr_t phys_addr, virt_addr_t virt_addr, uint8_t size)
{
	return false;
}
