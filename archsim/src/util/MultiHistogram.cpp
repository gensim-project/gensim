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
//    for (archsim::util::MultiHistogramIter I(*HI);
//         !I.is_end();
//         ++I)
//    {
//      fprintf(stderr, "%s\n", (*I)->to_string().c_str());
//    }
// }
//
// =====================================================================

#include "util/MultiHistogram.h"

#include "util/Histogram.h"

#include "concurrent/ScopedLock.h"

#include <sstream>

namespace archsim
{
	namespace util
	{

// -------------------------------------------------------------------------
// HistogramEntryComparator implementation
//
		bool HistogramComparator::operator()(const Histogram* lhs, const Histogram* rhs) const
		{
			return lhs->get_id() < rhs->get_id();
		}

// -------------------------------------------------------------------------
// MultiHistogram
//

// Re-fill HistogramEntry allocation pool
//
		uint32_t MultiHistogram::refill_historgram_allocation_pool()
		{
			// Register pointer to Histogram chunk
			//
			Histogram* entries = new Histogram[alloc_size_];
			multihist_entries_alloc_chunks_.push(entries);

			// Fill up hist_alloc_pool_
			//
			for (uint32_t i = 0; i < alloc_size_; ++i) {
				multihist_entries_alloc_pool_.push(&entries[i]);
			}

			return multihist_entries_alloc_pool_.size();
		}

// Returns pointer to new Histogram while maintaining all lookup structures
//
		Histogram* MultiHistogram::new_histogram(uint32_t idx)
		{
			archsim::concurrent::ScopedLock lock(multihist_entries_mutex_);

			// Re-fill allocation pool if it is empty
			//
			if (multihist_entries_alloc_pool_.empty()) {
				refill_historgram_allocation_pool();
			}

			// Pop entry from allocation pool stack
			//
			Histogram* entry = multihist_entries_alloc_pool_.top();
			multihist_entries_alloc_pool_.pop();

			// Set Histogram ID desired index
			//
			entry->set_id(idx);

			// Add entry to histogram entries
			//
			multihist_entries_.insert(entry);

			// Install mapping into fast lookup map
			//
			multihist_map_index_value_[idx] = entry;

			return entry;
		}

// Retrieve Histogram pointer at index
//
		const Histogram* MultiHistogram::get_hist_ptr_at_index(uint32_t idx)
		{
			Histogram* h = 0;
			std::map<uint32_t, Histogram*>::const_iterator I;
			if ((I = multihist_map_index_value_.find(idx)) != multihist_map_index_value_.end()) {
				// FAST PATH ------------------------------------------------------------
				//

				h = I->second;
			} else {
				// SLOW PATH ------------------------------------------------------------
				//

				// Histogram is not in fast look-up map so we have to create and install
				// a new one via the following method call.
				//
				h = new_histogram(idx);
			}
			return h;
		}

// Check if entry exists at index
//
		bool MultiHistogram::index_exists(uint32_t idx) const
		{
			return multihist_map_index_value_.find(idx) != multihist_map_index_value_.end();
		}

// Check if Histogram exists and HistogramEntry exists at given index
//
		bool MultiHistogram::index_exists_in_histogram(uint32_t idx_hist, uint32_t idx_entry) const
		{
			std::map<uint32_t, Histogram*>::const_iterator I;
			if ((I = multihist_map_index_value_.find(idx_hist)) != multihist_map_index_value_.end()) {
				return I->second->index_exists(idx_entry);
			}
			return false;
		}

// Increment. If index does not exist it will be automatically allocated.
//
		void MultiHistogram::inc(uint32_t idx_hist, uint32_t idx_entry)
		{
			Histogram* h = 0;
			std::map<uint32_t, Histogram*>::const_iterator I;
			if ((I = multihist_map_index_value_.find(idx_hist)) != multihist_map_index_value_.end()) {
				// FAST PATH ------------------------------------------------------------
				//

				h = I->second;
			} else {
				// SLOW PATH ------------------------------------------------------------
				//

				// Histogram is not in fast look-up map so we have to create and install
				// a new one via the following method call.
				//
				h = new_histogram(idx_hist);
			}
			// Increment Histogram at idx_entry
			//
			h->inc(idx_entry);
		}

		void MultiHistogram::inc(uint32_t idx_hist, uint32_t idx_entry, uint32_t val)
		{
			Histogram* h = 0;
			std::map<uint32_t, Histogram*>::const_iterator I;
			if ((I = multihist_map_index_value_.find(idx_hist)) != multihist_map_index_value_.end()) {
				// FAST PATH ------------------------------------------------------------
				//

				h = I->second;
			} else {
				// SLOW PATH ------------------------------------------------------------
				//

				// Histogram is not in fast look-up map so we have to create and install
				// a new one via the following method call.
				//
				h = new_histogram(idx_hist);
			}
			// Increment Histogram at idx_entry
			//
			h->inc(idx_entry, val);
		}

// Reset everything to 0
//
		void MultiHistogram::clear()
		{
			for (std::set<Histogram*>::const_iterator I = multihist_entries_.begin(), E = multihist_entries_.end(); I != E; ++I) {
				(*I)->clear();
			}
		}

		std::string MultiHistogram::to_string()
		{
			std::set<Histogram*, HistogramComparator> multihist_entries_sorted(multihist_entries_.begin(), multihist_entries_.end());

			std::ostringstream buf;

			for (std::set<Histogram*>::iterator I = multihist_entries_sorted.begin(), E = multihist_entries_sorted.end(); I != E; ++I) {
				buf << "\n\t[IDX:" << (*I)->get_id() << "]" << (*I)->to_string();
			}
			return buf.str();
		}

// Constructor
//
		MultiHistogram::MultiHistogram(uint32_t alloc_size) : alloc_size_(alloc_size)
		{
		}

// Destructor
//
		MultiHistogram::~MultiHistogram()
		{
			// Remove all dynamically allocated histogram chunks
			//
			while (!multihist_entries_alloc_chunks_.empty()) {
				Histogram* entries = multihist_entries_alloc_chunks_.top();
				delete[] entries;
				multihist_entries_alloc_chunks_.pop();
			}
		}

// -------------------------------------------------------------------------
// MultiHistogramIter
//
		MultiHistogramIter::MultiHistogramIter(const MultiHistogram& h) : hist(h)
		{
			entries.insert(h.multihist_entries_.begin(), h.multihist_entries_.end());
			iter = entries.begin();
		}

		void MultiHistogramIter::operator++()
		{
			++iter;
		}

		const Histogram* MultiHistogramIter::operator*()
		{
			return *iter;
		}

		bool MultiHistogramIter::is_end()
		{
			return iter == entries.end();
		}

		bool MultiHistogramIter::is_begin()
		{
			return iter == entries.begin();
		}
	}
}  // namespace archsim::util
