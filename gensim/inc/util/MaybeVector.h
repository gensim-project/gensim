/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MaybeVector.h
 * Author: harry
 *
 * Created on 29 September 2017, 15:05
 */

#ifndef MAYBEVECTOR_H
#define MAYBEVECTOR_H

#include <array>
#include <initializer_list>
#include <vector>
#include <cstddef>

#include "define.h"

namespace gensim {
	namespace util {
		template<typename T, int fixed_count> class MaybeVector {
		public:
			typedef MaybeVector<T, fixed_count> this_t;
			
			MaybeVector() : fixed_counter_(0), vector_(nullptr) {
			
			}
			
			MaybeVector(std::initializer_list<T> initialiser_list) : MaybeVector() { 
				if(initialiser_list.size() > fixed_count) {
					fixed_counter_ = fixed_count;
					vector_ = new std::vector<T>(initialiser_list);
				} else {
					for(auto &i : initialiser_list) { 
						push_back(i); 
					}
				}
			}
			
			MaybeVector(const this_t &other) : MaybeVector() { for(auto &i : other) { push_back(i); } }
			MaybeVector &operator=(const this_t &other) { 
				// TODO: this could be optimised
				clear();
				for(auto i : other) {
					push_back(i);
				}
				return *this;
			}
			
			class MBIterator {
			public:
				typedef ptrdiff_t difference_type; //almost always ptrdiff_t
				typedef T value_type; //almost always T
				typedef T& reference; //almost always T& or const T&
				typedef T* pointer; //almost always T* or const T*
				typedef std::input_iterator_tag iterator_category;  //usually std::forward_iterator_tag or similar
				
				MBIterator(this_t &parent, int start_it) : parent_vector_(parent), iterator_(start_it) {}
				bool operator !=(const MBIterator &other) { return (&parent_vector_ != &other.parent_vector_) || (iterator_ != other.iterator_); }
				bool operator ==(const MBIterator &other) { return (&parent_vector_ == &other.parent_vector_) && (iterator_ == other.iterator_); }
				
				// pre-increment
				MBIterator &operator++() { iterator_++; return *this; }
				
				// post-increment
				MBIterator &operator++(int) { MBIterator other(*this); iterator_++; return *other; }
				
				T &operator *() { return parent_vector_.at(iterator_); }
				T *operator->() { return &parent_vector_.at(iterator_); }
				
			private:
				this_t &parent_vector_;
				int iterator_;
			};
			class MBConstIterator {
			public:
				typedef ptrdiff_t difference_type; //almost always ptrdiff_t
				typedef T value_type; //almost always T
				typedef const T& reference; //almost always T& or const T&
				typedef const T* pointer; //almost always T* or const T*
				typedef std::input_iterator_tag iterator_category;  //usually std::forward_iterator_tag or similar
				
				
				MBConstIterator(const this_t &parent, int start_it) : parent_vector_(parent), iterator_(start_it) {}
				bool operator !=(const MBConstIterator &other) { return (&parent_vector_ != &other.parent_vector_) || (iterator_ != other.iterator_); }
				bool operator ==(const MBConstIterator &other) { return (&parent_vector_ == &other.parent_vector_) && (iterator_ == other.iterator_); }
				
				// pre-increment
				MBConstIterator &operator++() { iterator_++; return *this; }

				// post-increment
				MBIterator &operator++(int) { MBIterator other(*this); iterator_++; return *other; }
				
				const T &operator *() { return parent_vector_.at(iterator_); }
				T *operator->() { return &parent_vector_.at(iterator_); }
				
			private:
				const this_t &parent_vector_;
				int iterator_;
			};
			
			typedef MBIterator iterator_t;
			typedef MBConstIterator const_iterator_t;
			
			~MaybeVector() {
				if(is_a_vector()) {
					delete vector_;
				}
			}
			
			iterator_t begin() {
				return MBIterator(*this, 0);
			}
			iterator_t end() {
				return MBIterator(*this, size());
			}
			
			const_iterator_t begin() const {
				return MBConstIterator(*this, 0);
			}
			const_iterator_t end() const {
				return MBConstIterator(*this, size());
			}
			
			T& front() { return at(0); }
			const T& front() const { return at(0); }
			
			unsigned size() const { if(is_a_vector()) { return vector_->size(); } else { return fixed_counter_; } }
			bool empty() const { return size() == 0; }
			
			void clear() {
				if(is_a_vector()) {
					delete vector_;
					vector_ = nullptr;
				}
				fixed_counter_ = 0;
			}
			
			void push_back(T val) {
				if(fixed_counter_ == fixed_count) {
					if(!is_a_vector()) {
						convert_to_vector();
					}
					vector_->push_back(val);
				} else {
					fixed_array_[fixed_counter_] = val;
					fixed_counter_++;
				}
			}
			
			const T &at(unsigned i) const {
				if(is_a_vector()) {
					return vector_->at(i);
				} else {
					return fixed_array_.at(i);
				}
			}
			
			T &at(unsigned i) {
				if(is_a_vector()) {
					return vector_->at(i);
				} else {
					return fixed_array_.at(i);
				}
			}
			
			operator std::vector<T>() const {
				if(is_a_vector()) {
					return *vector_;
				} else {
					return std::vector<T> ( fixed_array_.begin(), fixed_array_.begin() + fixed_counter_);
				}
			}
			
		private:
			bool is_a_vector() const {
				return vector_ != nullptr;
			}
			
			void convert_to_vector() {
				vector_ = new std::vector<T>(fixed_array_.begin(), fixed_array_.begin() + fixed_counter_);
			}
			
			int fixed_counter_;
			
			std::array<T, fixed_count> fixed_array_;
			std::vector<T> *vector_;
		};
	}
}

#endif /* MAYBEVECTOR_H */

