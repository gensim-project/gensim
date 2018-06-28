/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   linked-vector.h
 * Author: harry
 *
 * Created on 06 June 2018, 16:19
 */

#ifndef LINKED_VECTOR_H
#define LINKED_VECTOR_H

#include <list>
#include <vector>

namespace archsim
{
	namespace util
	{

		/*
		 * A class which is a 'linked vector': a linked list where elements can
		 * also be stored contiguously. Is intended to give access properties
		 * similar to vectors (fast access and insertion at end) and linked
		 * lists (fast insertion and removal in the middle).
		 */
		template<typename T> class linked_vector
		{
		public:
			// Right now we're using vectors and lists, in the future we should
			// use lists and some sort of custom buffer.
			using this_t = linked_vector<T>;
			using inner_storage_t = std::vector<T>;
			using outer_storage_t = std::list<inner_storage_t>;

			class forward_iterator
			{
			public:
				forward_iterator(this_t *v) : outer_iterator_(v->storage_.begin()), inner_iterator_(outer_iterator_->begin()), outer_end_(v->storage.end()) { }
				forward_iterator(const forward_iterator &other) : outer_iterator_(other.outer_iterator_), inner_iterator_(other.inner_iterator_), outer_end_(other.outer_end_) {}

				T& operator*()
				{
					return *inner_iterator_;
				}

				forward_iterator operator++()
				{
					forward_iterator other = *this;
					advance(1);
					return other;
				}
				forward_iterator operator+(unsigned amount)
				{
					forward_iterator other = *this;
					other.advance(amount);
					return other;
				}

				bool operator==(const forward_iterator &other)
				{
					// if we're an 'end' iterator, check to see if the other iterator is an 'end' iterator
					if(is_end() && other.is_end()) {
						return true;
					}

					// otherwise check to see if the main iterators match.
					return outer_iterator_ == other.outer_iterator_ && inner_iterator_ == other.inner_iterator_;
				}

			private:
				typename outer_storage_t::iterator outer_iterator_;
				typename inner_storage_t::iterator inner_iterator_;
				typename outer_storage_t::iterator outer_end_;

				bool is_end()
				{
					return outer_iterator_ == outer_end_;
				}

				void advance(int count)
				{
					while(count--) {
						advance();
					}
				}

				void advance()
				{
					// advance inner iterator. if it reaches outer_iterator_->end, then advance outer iterator and reset inner iterator
					// if we reach outer_end, then don't reset inner_iterator.
					inner_iterator_++;
					if(inner_iterator_ == outer_iterator_->end()) {
						outer_iterator_++;
						if(outer_iterator_ != outer_end_) {
							inner_iterator_ = outer_iterator_->begin();
						}
					}
				}
			};

			size_t size()
			{
				unsigned size = 0;
				for(auto i : storage_) {
					size += i->size();
				}
				return size;
			}

			T& at(unsigned idx)
			{
				return *(begin() + idx);
			}

			void push_back(const T& val)
			{
				storage_.back().push_back(val);
			}
			T& back()
			{
				return storage_.back().back();
			}

			forward_iterator begin()
			{
				return forward_iterator(this);
			}
			forward_iterator end()
			{
				return forward_iterator(this, storage_.end());
			}

			void insert(const forward_iterator &iterator, const T& value)
			{
				iterator.outer_iterator_->insert(iterator.inner_iterator_, value);
			}

		private:
			outer_storage_t storage_;
		};

	}
}

#endif /* LINKED_VECTOR_H */
