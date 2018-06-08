/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   vbitset.h
 * Author: harry
 *
 * Created on 23 May 2018, 10:08
 */

#ifndef VBITSET_H
#define VBITSET_H

#include <vector>

namespace archsim {
	namespace util {
		class vbitset {
		public:
			vbitset(unsigned int size) : storage_(size, false) {}
			vbitset(unsigned int size, uint64_t value) : storage_(size, false) 
			{
				for(int i = 0; i < size; ++i) {
					set(i, value & (1 << i));
				}
			}
			
			bool get(unsigned int i) const { return storage_.at(i); }
			void set(unsigned int i, bool value) { storage_[i] = value; }
			void set_all() { for(int i = 0; i < size(); ++i) { set(i, 1); } }
			
			int size() const { return storage_.size(); }
			
			int get_lowest_set() const { for(int i = 0; i < size(); ++i) { if(get(i)) { return i; } } return -1; }
			int get_lowest_clear() const { for(int i = 0; i < size(); ++i) { if(!get(i)) { return i; } } return -1; }
			
			bool all_set() const { for(int i = 0; i < size(); ++i) { if(!get(i)) { return false; } } return true; }
			bool all_clear() const { for(int i = 0; i < size(); ++i) { if(get(i)) { return false; } } return true; }
			
			void invert() { for(int i = 0; i < size(); ++i) { set(i, !get(i)); } }
			vbitset inverted() const { auto other = *this; other.invert(); return other; }
			void clear() { for(int i = 0; i < size(); ++i) { set(i, false); } }
			
			int count() { int count = 0; for(int i = 0; i < size(); ++i) { count += get(i); } return count; }
			
		private:
			std::vector<bool> storage_;
		};
	}
}

#endif /* VBITSET_H */

