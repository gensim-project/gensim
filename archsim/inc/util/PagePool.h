/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PagePool.h
 *
 *  Created on: 6 Nov 2014
 *      Author: harry
 */

#ifndef PAGEPOOL_H_
#define PAGEPOOL_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <vector>
#include <unordered_set>

namespace archsim
{
	namespace util
	{

		class PagePool;
		class PageSet;
		class PageReference;

		class PageSet
		{
		public:
			PageSet(PagePool &parent);
			~PageSet();

			bool CanAllocate(long int NumPages, bool can_coalesce);
			void *Get(long int NumPages, bool can_coalesce);
			void Return(void *ptr, size_t NumPages);

			bool Empty() const
			{
				if(free_regions.size() != 1) {
					return false;
				}
				return free_regions[0].NumPages == kNumPages;
			}

			static const uint32_t kPageSize = 4096;
			static const uint32_t kNumPages = 1024;
			static const uint32_t kTotalSize = kPageSize * kNumPages;

		private:
			friend class PagePool;

			struct FreePageSet {
				void *Start;
				long int NumPages;
			};
			static bool FreePageSetCmp(const FreePageSet &a, const FreePageSet &b);

			std::vector<FreePageSet> free_regions;
			PagePool &Parent;
			void *Start;

			void Coalesce();

			//Coalesce the two page sets together if they are contiguous
			//Return true if the regions were coalesced
			bool Coalesce(FreePageSet &first, FreePageSet &second);

		};

		class PagePool
		{
		public:
			PagePool();

			PageReference *Allocate(size_t num_pages);
			PageReference *AllocateB(size_t num_bytes)
			{
				size_t pages = 1 + (num_bytes / PageSet::kPageSize);
//		fprintf(stderr, "*** ALLOCING %lu pages for %lu bytes (%lu)\n", pages, num_bytes, pages * PageSet::kPageSize);
				return Allocate(pages);
			}
			void GC();

		private:
			PageSet *curr_set;
			std::unordered_set<PageSet*> page_sets;
			std::mutex lock;
		};


		class PageReference
		{
		public:
			~PageReference();
			void * const Data;
			size_t const Num_Pages;

			size_t Size()
			{
				return Num_Pages * PageSet::kPageSize;
			}
		private:
			friend class PagePool;
			PageSet &Parent;

			PageReference(PageSet &parent, void *Data, size_t Num_Pages);
		};


	}
}




#endif /* PAGEPOOL_H_ */
