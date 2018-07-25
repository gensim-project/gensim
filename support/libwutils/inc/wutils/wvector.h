/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   wvector.h
 * Author: harry
 *
 * Created on 25 July 2018, 13:13
 */

#ifndef WVECTOR_H
#define WVECTOR_H

namespace wutils
{
	template<typename T, typename Allocator> class wvector
	{
	public:
		using iterator = T*;
		using wsize_t = unsigned long;

		wvector() : storage_(nullptr), size_(0), capacity_(0) {}

		wvector(wsize_t size) : storage_(nullptr), size_(0), capacity_(0)
		{
			resize(size);
		}
		wvector(wsize_t size, const T& value) : storage_(nullptr), size_(0), capacity_(0)
		{
			resize(size, value);
		}

		~wvector()
		{
			for(wsize_t i = 0; i < size_; ++i) {
				storage_[i].~T();
			}
		}

		iterator begin()
		{
			return iterator(storage_);
		}
		iterator end()
		{
			return iterator(storage_ + size_);
		}

		void push_back(const T& t)
		{
			ensure_capacity(size_+1);
			storage_[size_] = t;
			size_++;
		}
		void pop_back()
		{
			size_--;
		}

		T &front()
		{
			return at(0);
		}
		T &back()
		{
			return at(size()-1);
		}

		T& operator[](wsize_t index)
		{
			return storage_[index];
		}
		const T& operator[](wsize_t index) const
		{
			return storage_[index];
		}
		T& at(wsize_t index)
		{
			if(index >= size_) {
				abort();
			}
			return storage_[index];
		}
		const T& at(wsize_t index) const
		{
			if(index >= size_) {
				abort();
			}
			return storage_[index];
		}

		void resize(wsize_t new_size, const T& value = T())
		{
			ensure_capacity(new_size);
			while(size_ < new_size) {
				storage_[size_] = value;
				size_++;
			}
		}

		void trim()
		{
			storage_ = allocator_.realloc(storage_, size_);
			capacity_ = size_;
		}

		unsigned long size() const
		{
			return size_;
		}
		char *data()
		{
			return storage_;
		}

	private:
		void ensure_capacity(int new_capacity)
		{
			bool resize = false;

			if(capacity_ == 0) {
				capacity_ = 1;
				resize = true;
			}

			while(capacity_ < new_capacity) {
				capacity_ *= 2;
				resize = true;
			}
			if(resize) {
				storage_ = (T*)allocator_.realloc(storage_, new_capacity * sizeof(T));
			}
		}

		unsigned long size_, capacity_;
		Allocator allocator_;
		T *storage_;
	};
}

#endif /* WVECTOR_H */

