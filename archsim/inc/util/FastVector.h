/*
 * FastVector.h
 *
 *  Created on: 6 Feb 2014
 *      Author: harry
 */

#ifndef FASTVECTOR_H_
#define FASTVECTOR_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>


namespace util
{
	template<typename T, int Num, typename OverflowType = std::vector<T> > class FastVector
	{
	public:

		class FastVectorIterator : std::iterator<std::forward_iterator_tag, T>
		{
			friend class FastVectorConstIterator;
			typedef FastVector<T, Num, OverflowType> container_t;
		public:
			container_t &container;
			size_t position;

			FastVectorIterator(container_t &container, size_t pos = 0) : container(container), position(pos) {}
			FastVectorIterator(const FastVectorIterator &other) : container(other.container), position(other.position) {}

			FastVectorIterator &operator++()
			{
				++position;
				return *this;
			}
			FastVectorIterator operator++(int)
			{
				FastVectorIterator other(*this);
				operator++();
				return other;
			}

			bool operator==(const FastVectorIterator &other)
			{
				return (&container == &other.container) && (position == other.position);
			}
			bool operator!=(const FastVectorIterator &other)
			{
				return !operator==(other);
			}

			T& operator*()
			{
				return container.at(position);
			}
		};

		class FastVectorConstIterator : std::iterator<std::forward_iterator_tag, const T>
		{
			friend class FastVectorIterator;
			typedef FastVector<T, Num, OverflowType> container_t;
		public:
			const container_t &container;
			size_t position;

			FastVectorConstIterator(const container_t &container, size_t pos = 0) : container(container), position(pos) {}
			FastVectorConstIterator(const FastVectorIterator &other) : container(other.container), position(other.position) {}
			FastVectorConstIterator(const FastVectorConstIterator &other) : container(other.container), position(other.position) {}

			FastVectorConstIterator &operator++()
			{
				++position;
				return *this;
			}
			FastVectorConstIterator operator++(int)
			{
				FastVectorConstIterator other(*this);
				operator++();
				return other;
			}

			bool operator==(const FastVectorConstIterator &other)
			{
				return (&container == &other.container) && (position == other.position);
			}
			bool operator!=(const FastVectorConstIterator &other)
			{
				return !operator==(other);
			}

			const T& operator*()
			{
				return container.at(position);
			}
		};

		typedef T value_type;
		typedef value_type &reference;
		typedef const T const_value_type;
		typedef const T &const_reference;
		typedef FastVectorIterator iterator;
		typedef FastVectorConstIterator const_iterator;

		FastVector() : count(0) {}
		~FastVector()
		{
		}

		void push_back(const value_type &val)
		{
			if(count == Num) switch_to_vector();
			if(count >= Num) push_back_vector(val);
			else push_back_values(val);
		}

		size_t size() const
		{
			return count;
		}
		void resize(const size_t new_size)
		{
			if(new_size > Num) {
				switch_to_vector();
				vector.resize(new_size);
			}
		}
		bool empty() const
		{
			return count > 0;
		}
		void reserve(const size_t reserve_size)
		{
			if(reserve_size > Num) {
				switch_to_vector();
				vector.reserve(reserve_size);
			}
		}

		const_reference at(const size_t i) const
		{
			if(using_values()) return values[i];
			else return vector.at(i);

			__builtin_unreachable();
		}

		reference at(const size_t i)
		{
			if(using_values()) return values[i];
			else return vector.at(i);

			__builtin_unreachable();
		}

		reference front()
		{
			if(using_values()) return values[0];
			else return vector.front();

			__builtin_unreachable();
		}

		const_reference front()const
		{
			if(using_values()) return values[0];
			else return vector.front();

			__builtin_unreachable();
		}


		reference back()
		{
			if(using_values()) return values[0];
			else return vector.back();

			__builtin_unreachable();
		}

		const_reference back() const
		{
			if(using_values()) return values[0];
			else return vector.back();

			__builtin_unreachable();
		}

		FastVectorIterator begin()
		{
			return FastVectorIterator(*this, 0);
		}
		FastVectorIterator end()
		{
			return FastVectorIterator(*this, count);
		}

		FastVectorConstIterator begin() const
		{
			return FastVectorConstIterator(*this, 0);
		}
		FastVectorConstIterator end() const
		{
			return FastVectorConstIterator(*this, count);
		}

		void clear()
		{
			if(using_vector()) {
				vector.clear();
			}
			count = 0;
		}

	private:
		union {
			T values[Num];
			OverflowType vector;
		};
		uint32_t count;

		void switch_to_vector()
		{
			OverflowType v;
			v.reserve(count);
			for(int i = 0; i < Num; ++i) v.push_back(values[i]);
			vector = v;
		}

		void push_back_values(const value_type &v)
		{
			values[count] = v;
			count++;
		}

		void push_back_vector(const value_type &v)
		{
			vector.push_back(v);
			count++;
		}

		bool using_values() const
		{
			return count <= Num;
		}
		bool using_vector() const
		{
			return count > Num;
		}
	};
}


#endif /* FASTVECTOR_H_ */
