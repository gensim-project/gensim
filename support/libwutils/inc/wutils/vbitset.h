/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/* 
 * File:   vbitset.h
 * Author: harry
 *
 * Created on 23 May 2018, 10:08
 */

#ifndef VBITSET_H
#define VBITSET_H

#include <vector>

namespace wutils {
	class vbitset {
	public:
		vbitset(unsigned int size) : storage_(size, false) {}
		vbitset(unsigned int size, uint64_t value) : storage_(size, false) 
		{
			for(unsigned int i = 0; i < size; ++i) {
				set(i, value & (1 << i));
			}
		}

		bool get(unsigned int i) const { return storage_.at(i); }
		void set(unsigned int i, bool value) { storage_[i] = value; }
		void set_all() { for(unsigned int i = 0; i < size(); ++i) { set(i, 1); } }

		unsigned int size() const { return storage_.size(); }

		int get_lowest_set() const { for(unsigned int i = 0; i < size(); ++i) { if(get(i)) { return i; } } return -1; }
		int get_lowest_clear() const { for(unsigned int i = 0; i < size(); ++i) { if(!get(i)) { return i; } } return -1; }

		bool all_set() const { for(unsigned int i = 0; i < size(); ++i) { if(!get(i)) { return false; } } return true; }
		bool all_clear() const { for(unsigned int i = 0; i < size(); ++i) { if(get(i)) { return false; } } return true; }

		void invert() { for(unsigned int i = 0; i < size(); ++i) { set(i, !get(i)); } }
		vbitset inverted() const { auto other = *this; other.invert(); return other; }
		void clear() { for(unsigned int i = 0; i < size(); ++i) { set(i, false); } }

		int count() { int count = 0; for(unsigned int i = 0; i < size(); ++i) { count += get(i); } return count; }

		bool operator==(const vbitset &other) const {
			if(size() != other.size()) {
				return false;
			}
			for(unsigned int i = 0; i < size(); ++i) {
				if(get(i) != other.get(i)) {
					return false;
				}
			}
			return true;
		}
		bool operator!=(const vbitset &other) const {
			return !(operator==(other));
		}
		
		void operator&=(const vbitset &other) {
			if(size() != other.size()) {
				throw std::range_error("Cannot merge bitsets of different sizes");
			}
			for(unsigned int i = 0; i < size(); ++i) {
				set(i, get(i) & other.get(i));
			}
		}
		
	private:
		std::vector<bool> storage_;
	};
}

#endif /* VBITSET_H */

