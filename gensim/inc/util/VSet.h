/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   VSet.h
 * Author: harry
 *
 * Created on 23 June 2017, 15:12
 */

#ifndef VSET_H
#define VSET_H

#include <vector>

namespace gensim
{
	namespace util
	{

		template<typename T> class VSet
		{
		public:
			typedef typename std::vector<T> storage_t;
			typedef typename storage_t::iterator iterator;
			typedef typename storage_t::const_iterator const_iterator;
			typedef T value_type;
			typedef VSet<T> this_t;

			void insert(const T& t)
			{
				if(!count(t)) {
					storage_.push_back(t);
				}
			}

			template<typename iterator_type> void insert(iterator_type begin_it, iterator_type end_it)
			{
				while(begin_it != end_it) {
					insert(*begin_it);
					begin_it++;
				}
			}

			iterator insert(iterator it, const T& t)
			{
				return storage_.insert(it, t);
			}

			unsigned count(const T& t) const
			{
				for(const auto &i : storage_) {
					if(i == t) {
						return 1;
					}
				}
				return 0;
			}

			unsigned size() const
			{
				return storage_.size();
			}

			iterator find(const T& t)
			{
				for(auto it = begin(); it != end(); ++it) {
					if(*it == t) {
						return it;
					}
				}
				return end();
			}

			void erase(const T& t)
			{
				erase(find(t));
			}

			void erase(const iterator &t)
			{
				storage_.erase(t);
			}

			iterator begin()
			{
				return storage_.begin();
			}

			iterator end()
			{
				return storage_.end();
			}

			const_iterator begin() const
			{
				return storage_.begin();
			}

			const_iterator end() const
			{
				return storage_.end();
			}

			bool operator==(const this_t &other) const
			{
				if(size() != other.size()) {
					return false;
				}
				for(const auto &i : storage_) {
					if(!other.count(i)) {
						return false;
					}
				}
				return true;
			}

			bool operator!=(const this_t &other) const
			{
				return !(operator==(other));
			}

		private:
			storage_t storage_;
		};

	}
}

#endif /* VSET_H */

