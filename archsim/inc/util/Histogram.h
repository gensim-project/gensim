//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe Histogram classes responsible for maintaining and
// instantiating all kinds of profiling counters in a generic way.
//
// A Histogram class consists of HistogramEntries and allows to calculate
// the frequencies of just about anything.
//
// The Histogram is extremely powerful, efficient and easy to use. It grows
// on demand and fully automatic and it integrates very well with our JIT
// compiler.
//
// There is also a HistogramIter iterator that allows for easy iteration
// over histogram entries in a Histogram:
//
//
// for (archsim::util::HistogramIter I(histogram_instance);
//      !I.is_end();
//      ++I)
// {
//   fprintf(stderr, "%s\n", (*I)->to_string().c_str());
// }
//
// FIXME:
//  - Introduce method to 'Histogram' that will return the next non-zero address.
//
//
// =====================================================================

#ifndef INC_UTIL_HISTOGRAM_H_
#define INC_UTIL_HISTOGRAM_H_

#include "concurrent/Mutex.h"

#include <set>
#include <map>
#include <stack>
#include <string>

namespace archsim
{
	namespace util
	{

// -------------------------------------------------------------------------
// Forward declarations
//
		class MultiHistogram;

// -------------------------------------------------------------------------
// HistogramEntry is used to encapsulate a single 'bar' of a histogram.
//
		class HistogramEntry
		{
		public:
			typedef uint32_t histogram_key_t;
			typedef uint64_t histogram_value_t;

			// Constructor
			//
			HistogramEntry();

			void set_value(histogram_value_t val)
			{
				y_value_ = val;
			}
			histogram_value_t get_value() const
			{
				return y_value_;
			}
			histogram_value_t* get_value_ptr()
			{
				return &y_value_;
			}

			void set_index(histogram_key_t idx)
			{
				x_index_ = idx;
			}
			histogram_key_t get_index() const
			{
				return x_index_;
			}

			void inc()
			{
				++y_value_;
			}
			void inc(histogram_value_t val)
			{
				y_value_ += val;
			}

			// Reset to 0
			//
			void clear()
			{
				y_value_ = 0;
			}

			// Return 'name: value' as string
			//
			std::string to_string() const;

			// Return data formatted according to format string
			//
			std::string to_string(const char* format) const;

		private:
			histogram_key_t x_index_;
			histogram_value_t y_value_;
		};

// -------------------------------------------------------------------------
// HistogramEntryComparator implementation
//
		struct HistogramEntryComparator {
			bool operator()(const HistogramEntry* lhs, const HistogramEntry* rhs) const
			{
				return lhs->get_index() < rhs->get_index();
			}
		};

// -------------------------------------------------------------------------
// Histogram  maintains HistogramEntries allowing fast access and
// increment operations.
//
		class Histogram
		{
			// MultiHistogram and HistogramIter are our friends and get to access
			// private members and methods
			//
			friend class MultiHistogram;
			friend class HistogramIter;

		protected:
			// The following methods are not 'publicly' exposed as we do not want
			// to allow people to modify Histogram IDs and Histogram names after
			// constructor initialisation. We only allow it in cases where the
			// Histogram is embedded in another container, such as the MultiHistogram,
			// where system wide unique Histogram IDs do not make sense because in that
			// case the MultiHistogram is container (i.e. CounterManager) managed.
			//

			// No args constructor creating an empty Histogram without unique ID
			// assignment and a name
			//
			// Set histogram ID
			//
			void set_id(uint32_t id)
			{
				id_ = id;
			}

		public:
			typedef std::map<HistogramEntry::histogram_key_t, HistogramEntry::histogram_value_t*> hist_map_key_ptr_t;

			// Default HistogramEntry allocation size
			//
			static const int kHistogramEntryDefaultAllocSize = 64;

			explicit Histogram();
			explicit Histogram(uint32_t alloc_size);

			// Destructor
			//
			~Histogram();

			uint32_t get_id() const
			{
				return id_;
			}

			// Set value at index. If index does not exist it will be automatically allocated.
			//
			void set_value_at_index(HistogramEntry::histogram_key_t idx, HistogramEntry::histogram_value_t val);

			// Get value at index. If index does not exist it will be automatically allocated.
			//
			HistogramEntry::histogram_value_t get_value_at_index(HistogramEntry::histogram_key_t idx);

			// Get pointer to value at index. If index does not exist it will be
			// automatically allocated.
			//
			HistogramEntry::histogram_value_t* get_value_ptr_at_index(HistogramEntry::histogram_key_t idx);

			// Check if entry exists at index
			//
			bool index_exists(HistogramEntry::histogram_key_t idx) const;

			// Sum all 'bars' in histogram
			//
			HistogramEntry::histogram_value_t get_total(void) const;

			// Increment. If index does not exist it will be automatically allocated.
			//
			void inc(HistogramEntry::histogram_key_t idx);
			void inc(HistogramEntry::histogram_key_t idx, HistogramEntry::histogram_value_t val);

			// Reset everything to 0
			//
			void clear();

			// Return histogram as string
			//
			std::string to_string();

			// Return histogram with entries formatted as specified
			std::string to_string(const char* const);

			inline const hist_map_key_ptr_t &get_value_map() const
			{
				return hist_map_index_value_;
			}

		private:
			uint32_t id_;
			static const uint32_t kInitialHistogramId = 0xFFFFFFFF;

			uint32_t alloc_size_;

			// Set of ALL HistogramEntries for this Histogram
			//
			std::set<HistogramEntry*> hist_entries_;

			// Map for fast lookup/increment of HistogramEntry values:
			// key: hist index
			// val: pointer to hist value
			//
			hist_map_key_ptr_t hist_map_index_value_;

			// In order to allow for allocation to happen in chunks we use an allocation
			// pool that looks like a stack. When adding a new HistogramEntry to the
			// hist_entries_ set we pop a pre-allocated element from the hist_alloc_pool_
			// stack. Should the hist_alloc_pool_ become empty we will allocate another
			// chunk of HistogramEntries and put them on a stack.
			//
			std::stack<HistogramEntry*> hist_entries_alloc_pool_;

			// Stack of ALL allocated HistogramEntry chunks
			//
			std::stack<HistogramEntry*> hist_entries_alloc_chunks_;

			// Mutex that must be acquired before modifying HistogramEntry book keeping
			// structures
			//
			archsim::concurrent::Mutex hist_entries_mutex_;

			// Re-fills allocation pool and returns number of instantiated HistogramEntries
			//
			uint32_t refill_historgram_allocation_pool();

			// Returns pointer to new HistogramEntry and maintains all lookup
			// structures
			//
			HistogramEntry* new_histogram_entry(HistogramEntry::histogram_key_t idx);
		};

// -------------------------------------------------------------------------
// HistogramIter iterator allowing easy iteration over HistogramEntries.
//
		class HistogramIter
		{
		private:
			const Histogram& hist;
			std::set<HistogramEntry*, HistogramEntryComparator> entries;
			std::set<HistogramEntry*>::const_iterator iter;

		public:
			HistogramIter(const Histogram& h);
			HistogramIter(Histogram* const h);
			HistogramIter(const HistogramIter& hi);
			HistogramIter(HistogramIter* const hi);
			void operator++();
			const HistogramEntry* operator*();
			bool is_begin();
			bool is_end();
		};
	}
}  // namespace archsim::util

#endif  // INC_UTIL_HISTOGRAM_H_
