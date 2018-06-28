/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "uarch/cache/CacheGeometry.h"
#include "util/LivePerformanceMeter.h"
#include "abi/EmulationModel.h"
#include "system.h"

using namespace archsim::uarch::cache;

CacheGeometry::CacheGeometry() { }

CacheGeometry::~CacheGeometry()
{
	for (auto cache : caches) {
		delete cache;
	}

	caches.clear();
}

static void cache_flush_handler(PubSubType::PubSubType type, void *context, const void *data)
{
	archsim::uarch::cache::MemoryCache *cache = (archsim::uarch::cache::MemoryCache *)context;
	cache->Flush();
}

bool CacheGeometry::Install(gensim::Processor& cpu, abi::memory::MemoryModel& model)
{
	archsim::uarch::cache::MemoryCache *l1d, *l1i, *l2, *mm;

	l1d = new archsim::uarch::cache::types::Associative("L1-D", 32 * 1024, 2, 64, archsim::uarch::cache::types::Associative::LRU, archsim::uarch::cache::types::Associative::Physical, archsim::uarch::cache::types::Associative::Physical);
	l1i = new archsim::uarch::cache::types::Associative("L1-I", 32 * 1024, 2, 64, archsim::uarch::cache::types::Associative::LRU, archsim::uarch::cache::types::Associative::Physical, archsim::uarch::cache::types::Associative::Physical);
	l2 = new archsim::uarch::cache::types::Associative("L2", 1 * 1024 * 1024, 16, 64, archsim::uarch::cache::types::Associative::Random, archsim::uarch::cache::types::Associative::Physical, archsim::uarch::cache::types::Associative::Physical);
	mm = new archsim::uarch::cache::types::MainMemory();

	caches.push_back(l1d);
	caches.push_back(l1i);
	caches.push_back(l2);
	caches.push_back(mm);

	l2->SetNext(archsim::uarch::cache::MemoryCache::InstructionFetch, *mm);
	l2->SetNext(archsim::uarch::cache::MemoryCache::DataRead, *mm);
	l2->SetNext(archsim::uarch::cache::MemoryCache::DataWrite, *mm);

	l1d->SetNext(archsim::uarch::cache::MemoryCache::DataRead, *l2);
	l1d->SetNext(archsim::uarch::cache::MemoryCache::DataWrite, *l2);
	l1i->SetNext(archsim::uarch::cache::MemoryCache::InstructionFetch, *l2);

	event_handler.SetNext(archsim::uarch::cache::MemoryCache::DataRead, *l1d);
	event_handler.SetNext(archsim::uarch::cache::MemoryCache::DataWrite, *l1d);
	event_handler.SetNext(archsim::uarch::cache::MemoryCache::InstructionFetch, *l1i);

	model.RegisterEventHandler(event_handler);

	UNIMPLEMENTED;
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("L1-D Hits", l1d->GetHitCounter()));
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("L1-D Misses", l1d->GetMissCounter()));
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("L1-I Hits", l1i->GetHitCounter()));
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("L1-I Misses", l1i->GetMissCounter()));
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("L2 Hits", l2->GetHitCounter()));
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("L2 Misses", l2->GetMissCounter()));
//	cpu.GetEmulationModel().GetSystem().AddPerformanceSource(*new util::CounterPerformanceSource("MM Hits", mm->GetHitCounter()));
//
//	cpu.GetEmulationModel().GetSystem().GetPubSub().Subscribe(PubSubType::L1ICacheFlush, cache_flush_handler, l1i);
//	cpu.GetEmulationModel().GetSystem().GetPubSub().Subscribe(PubSubType::L1DCacheFlush, cache_flush_handler, l1d);
//	cpu.GetEmulationModel().GetSystem().GetPubSub().Subscribe(PubSubType::L2CacheFlush, cache_flush_handler, l2);

	return true;
}

void CacheGeometry::PrintStatistics(std::ostream& stream)
{
	stream << "Cache Geometry" << std::endl;
	event_handler.PrintGlobalCacheStatistics(stream);
}

void CacheGeometry::ResetStatistics()
{
	for (auto cache : caches) {
		cache->ResetStatistics();
	}
}
