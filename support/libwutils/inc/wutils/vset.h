/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   vset.h
 * Author: harry
 *
 * Created on 28 November 2018, 09:56
 */

#ifndef VSET_H
#define VSET_H

namespace wutils
{

	template<typename T, bool ordered=false, typename storage=std::vector<T>> class vset
	{
	public:
		using StorageT = storage;
		using ValueT = T;

		void insert(const T& t)
		{
			if(!count(t)) {
				storage_.push_back(t);
			}
		}

		template<typename it> void insert(it b, it e)
		{
			while(b != e) {
				insert(*b);
				b++;
			}
		}

		void erase(const T& t)
		{
			for(auto i = storage_.begin(); i < storage_.end(); ++i) {
				if(*i == t) {
					storage_.erase(i);
					return;
				}
			}
		}

		bool count(const T& t) const
		{
			for(const auto v : storage_) {
				if(v == t) {
					return true;
				}
			}
			return false;
		}

		size_t size() const
		{
			return storage_.size();
		}

		typename StorageT::iterator begin()
		{
			return storage_.begin();
		}
		typename StorageT::iterator end()
		{
			return storage_.end();
		}
		typename StorageT::const_iterator begin() const
		{
			return storage_.begin();
		}
		typename StorageT::const_iterator end() const
		{
			return storage_.end();
		}

		bool empty() const
		{
			return size() == 0;
		}

	private:
		void sort()
		{
			abort();
		}

		StorageT storage_;
	};

}

#endif /* VSET_H */

