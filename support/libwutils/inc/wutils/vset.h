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
		friend class vset<T, false, storage>;
		friend class vset<T, true, storage>;

		using ThisT = vset<T, ordered, storage>;
		using StorageT = storage;
		using ValueT = T;

		template<bool other_ordered> ThisT &operator=(const vset<T, other_ordered, storage> &other)
		{
			storage_ = other.storage_;
			if(ordered) {
				sort();
			}
			return *this;
		}

		vset() {}

		template<bool other_ordered> vset(const vset<T, other_ordered, storage> &other)
		{
			*this = other;
		}

		void insert(const T& t)
		{
			if(!count(t)) {
				storage_.push_back(t);
			}

			sort();
		}

		template<typename it> void insert(it b, it e)
		{
			while(b != e) {
				if(!count(*b)) {
					storage_.push_back(*b);
				}
				b++;
			}
			sort();
		}

		void erase(const T& t)
		{
			for(auto i = storage_.begin(); i < storage_.end(); ++i) {
				if(*i == t) {
					std::swap(*i, storage_.back());
					storage_.pop_back();
					break;
				}
			}
			sort();
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

		void reserve(int i)
		{
			storage_.reserve(i);
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
			if(ordered) {
				std::sort(storage_.begin(), storage_.end());
			}
		}

		StorageT storage_;
	};

}

#endif /* VSET_H */

