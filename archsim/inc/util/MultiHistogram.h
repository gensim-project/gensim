/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

// =====================================================================
//
// Description:
//
// Thread-safe MultiHistogram class is responsible for maintaining and
// instantiating profiling data that consist of multiple histograms (e.g.
// a three dimensional array)
//
// A MultiHistogram class consists of Histograms and allows to calculate
// the frequencies of just about anything.
//
// The MultiHistogram is extremely powerful, efficient and easy to use. It
// grows on demand and fully automatic and it integrates very well with our
// JIT compiler.
//
// There is also a MultiHistogramIter iterator that allows for easy iteration
// over Histograms in a MultiHistogram:
//
// // Iterate over all Histograms in MultiHistogram
// //
// for (archsim::util::MultiHistogramIter HI(multi_histogram_instance);
//      !HI.is_end();
//      ++HI)
// {
//    // Iterate over all HistogramEntries in Histogram
//    //
//    for (archsim::util::HistogramIter I(*HI);
//         !I.is_end();
//         ++I)
//    {
//      fprintf(stderr, "%s\n", (*I)->to_string().c_str());
//    }
// }
//
// =====================================================================

#ifndef INC_UTIL_MULTIHISTOGRAM_H_
#define INC_UTIL_MULTIHISTOGRAM_H_

#include "concurrent/Mutex.h"

#include <set>
#include <map>
#include <stack>
#include <string>
#include <cstdint>

namespace archsim
{
	namespace util
	{

// -------------------------------------------------------------------------
// Forward declarations
//
		class Histogram;

// -------------------------------------------------------------------------
// HistogramEntryComparator implementation
//
		struct HistogramComparator {
			bool operator()(const Histogram* lhs, const Histogram* rhs) const;
		};

// -------------------------------------------------------------------------
// MultiHistogram  maintains HistogramEntries allowing fast access and
// increment operations.
//
		class MultiHistogram
		{
			friend class MultiHistogramIter;

		public:
			// Default Histogram allocation size
			//
			static const int kHistogramDefaultAllocSize = 64;

			// Constructor
			//
			explicit MultiHistogram(uint32_t alloc_size = kHistogramDefaultAllocSize);

			// Histogram
			//
			~MultiHistogram();

			// Retrieve Histogram pointer at index
			//
			const Histogram* get_hist_ptr_at_index(uint32_t idx_hist);

			// Check if Histogram exists at index
			//
			bool index_exists(uint32_t idx) const;

			// Check if Histogram exists and HistogramEntry exists at given index
			//
			bool index_exists_in_histogram(uint32_t idx_hist, uint32_t idx_entry) const;

			// Increment. If index does not exist it will be automatically allocated.
			//
			void inc(uint32_t idx_hist, uint32_t idx_entry);
			void inc(uint32_t idx_hist, uint32_t idx_entry, uint32_t val);

			// Reset everything to 0
			//
			void clear();

			// Return histogram as string
			//
			std::string to_string();

		private:
			uint32_t alloc_size_;

			// Set of ALL Histograms for this MultiHistogram
			//
			std::set<Histogram*> multihist_entries_;

			// Map for fast lookup of Histograms:
			// key: hist index
			// val: pointer to histogram
			//
			std::map<uint32_t, Histogram*> multihist_map_index_value_;

			// In order to allow for allocation to happen in chunks we use an allocation
			// pool that looks like a stack. When adding a new Histogram to the
			// histograms_ set we pop a pre-allocated element from the hist_alloc_pool_
			// stack. Should the hist_alloc_pool_ become empty we will allocate another
			// chunk of HistogramEntries and put them on a stack.
			//
			std::stack<Histogram*> multihist_entries_alloc_pool_;

			// Stack of ALL allocated Histogram chunks
			//
			std::stack<Histogram*> multihist_entries_alloc_chunks_;

			// Mutex that must be acquired before modifying MultiHistogram book keeping
			// structures
			//
			archsim::concurrent::Mutex multihist_entries_mutex_;

			// Re-fills allocation pool and returns number of instantiated Histograms
			//
			uint32_t refill_historgram_allocation_pool();

			// Returns pointer to new Histogram and maintains all lookup structures
			//
			Histogram* new_histogram(uint32_t idx);
		};

// -------------------------------------------------------------------------
// MultiHistogramIter iterator allowing easy iteration over Histograms.
//
		class MultiHistogramIter
		{
		private:
			const MultiHistogram& hist;
			std::set<Histogram*, HistogramComparator> entries;
			std::set<Histogram*>::const_iterator iter;

		public:
			MultiHistogramIter(const MultiHistogram& h);
			void operator++();
			const Histogram* operator*();
			bool is_begin();
			bool is_end();
		};
	}
}  // namespace archsim::util

#endif  // INC_UTIL_MULTIHISTOGRAM_H_
