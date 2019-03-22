/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#ifndef WULIB_DENSE_SET_H
#define WULIB_DENSE_SET_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

namespace wutils
{
	template<typename T> class dense_set
	{
	public:
		typedef dense_set<T> this_t;

		class iterator
		{
		public:
			typedef std::forward_iterator_tag iterator_category;

			iterator(this_t *target) : _d(*target), _chunk_ptr(_d._backing_store), _chunk_copy(*_chunk_ptr), _counter(0)
			{
				next();
			}
			iterator(this_t *target, int counter) : _d(*target), _chunk_ptr(_d._backing_store), _chunk_copy(*_chunk_ptr), _counter(counter) {  }
			T operator*()
			{
				return _counter;
			}

			iterator operator++()
			{
				next();
				return *this;
			}
			iterator operator++(int)
			{
				iterator tmp(_d, _counter);
				next();
				return tmp;
			}

			bool operator==(const iterator &other)
			{
				return _counter == other._counter;
			}
			bool operator!=(const iterator &other)
			{
				return _counter != other._counter;
			}

		private:
			this_t &_d;
			uint64_t *_chunk_ptr;
			uint64_t _chunk_copy;
			T _counter;

			// TODO: this is really slow
			void next()
			{
				if(_chunk_copy) {
					uint64_t counter_base = _d.get_chunk_index(_counter)*_d.get_chunk_size_bits();

					uint64_t next_bit = __builtin_ctzl(_chunk_copy);
					_chunk_copy &= ~_d.get_mask(next_bit);
					_counter = counter_base + next_bit;
				} else {
					if(UNLIKELY(_chunk_ptr == _d.get_backing_store_end())) _counter = _d.size();
					else {
						while(!_chunk_copy && _chunk_ptr < _d.get_backing_store_end()) {
							_chunk_ptr++;
							_chunk_copy = *_chunk_ptr;
						}

						_counter = _d.get_chunk_size_bits() * (_chunk_ptr - _d._backing_store);
						next();
					}
				}
				if(UNLIKELY(_counter >= _d._size_bits)) _counter = _d._size_bits;
			}
		};



	public:
		friend class iterator;

		typedef iterator iterator_t;

		dense_set(size_t size_bits) : _size_bits(size_bits)
		{
			size_t size_in_chunks = get_chunk_count();

			_backing_store = new chunk_t[size_in_chunks];

			clear();
		}

		dense_set() = delete;

		dense_set(const dense_set &other) : _size_bits(other._size_bits)
		{
			size_t size_bytes = 1+(_size_bits / 8);
			size_t size_in_chunks = get_chunk_count();

			_backing_store = new chunk_t[size_in_chunks];
			clear();
			memcpy(_backing_store, other._backing_store, size_bytes);
		}

		dense_set(dense_set &&other)
		{
			_size_bits = other._size_bits;
			_backing_store = other._backing_store;
			other._backing_store = nullptr;
		}

		~dense_set()
		{
			delete [] _backing_store;
		}

		dense_set &operator=(const dense_set &other)
		{
			if(_backing_store) delete [] _backing_store;

			_size_bits = other._size_bits;
			size_t size_bytes = 1+(_size_bits / 8);
			size_t size_in_chunks = get_chunk_count();


			_backing_store = new chunk_t[size_in_chunks];
			clear();
			memcpy(_backing_store, other._backing_store, size_bytes);

			return *this;
		}

		iterator_t begin()
		{
			return iterator(this);
		}
		iterator_t end()
		{
			return iterator(this, size());
		}

		T first()
		{
			iterator i = begin();
			return *i;
		}

		//TODO: this is really slow
		T last() const
		{
			for(uint32_t i = size()-1; i >= 0; --i) if(count(i)) return i;
			return size();
		}

		void insert(T i)
		{
			assert(i < size());
			get_chunk(i) |= get_mask(i);
		}
		void insert(const dense_set<T> &i)
		{
			assert(i._size_bits == _size_bits);
			uint32_t chunk_count = get_chunk_count();
			for(uint32_t chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx) {
				_backing_store[chunk_idx] |= i._backing_store[chunk_idx];
			}
		}

		void copy(const dense_set<T> &i)
		{
			assert(i._size_bits == _size_bits);
			uint32_t chunk_count = get_chunk_count();
			for(uint32_t chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx) {
				_backing_store[chunk_idx] = i._backing_store[chunk_idx];
			}
		}

		uint32_t count(T i) const
		{
			assert(i < size());
			return (get_chunk(i) & get_mask(i)) != 0;
		}
		void erase(T i)
		{
			assert(i < size());
			get_chunk(i) &= ~get_mask(i);
		}

		uint32_t size() const
		{
			return _size_bits;
		}
		void clear()
		{
			bzero(_backing_store, get_chunk_count() * sizeof(_backing_store));
		}

	private:
		typedef uint64_t chunk_t;
		chunk_t *_backing_store;
		uint32_t _size_bits;

		chunk_t *get_backing_store_end()
		{
			return _backing_store + _size_bits / get_chunk_size_bits();
		}

		uint64_t &get_chunk(uint32_t index)
		{
			return _backing_store[index / get_chunk_size_bits()];
		}
		const uint64_t &get_chunk(uint32_t index) const
		{
			return _backing_store[index / get_chunk_size_bits()];
		}

		uint64_t get_chunk_index(uint32_t index) const
		{
			return index / get_chunk_size_bits();
		}
		uint64_t get_bit(uint32_t index) const
		{
			return index % get_chunk_size_bits();
		}
		uint64_t get_mask(uint32_t index) const
		{
			return 1ULL << get_bit(index);
		}

		size_t get_chunk_count() const
		{
			return 1+(_size_bits / get_chunk_size_bits());
		}
		size_t get_chunk_size_bits() const
		{
			return sizeof(*_backing_store)*8;
		}
		size_t get_chunk_size() const
		{
			return sizeof(*_backing_store);
		}

	};
}

#endif
