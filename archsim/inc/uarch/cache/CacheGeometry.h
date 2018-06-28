/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   CacheGeometry.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 13:10
 */

#ifndef CACHEGEOMETRY_H
#define	CACHEGEOMETRY_H

#include "uarch/cache/MemoryCacheEventHandler.h"

#include <ostream>
#include <list>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class MemoryModel;
		}
	}

	namespace uarch
	{
		namespace cache
		{
			class MemoryCache;

			class CacheGeometry
			{
			public:
				CacheGeometry();
				~CacheGeometry();

				bool Install(gensim::Processor& cpu, abi::memory::MemoryModel& model);
				void PrintStatistics(std::ostream& stream);
				void ResetStatistics();

			private:
				MemoryCacheEventHandler event_handler;
				std::list<MemoryCache *> caches;
			};
		}
	}
}

#endif	/* CACHEGEOMETRY_H */

