/*
 * DenseInterferenceMatrix.h
 *
 *  Created on: 16 May 2014
 *      Author: harry
 */

#ifndef DENSEINTERFERENCEMATRIX_H_
#define DENSEINTERFERENCEMATRIX_H_

#include <unordered_set>
#include <cstring>

namespace archsim
{
	namespace util
	{

		template <typename T, typename hash_fn_t> class DenseInterferenceMatrix
		{
		public:
			DenseInterferenceMatrix(uint32_t size) : size(size)
			{
				graph = (bool*)(malloc(size*size));
				bzero(graph, size*size);
				keys.resize(size);
				interference_counts.resize(size);
			}

			~DenseInterferenceMatrix()
			{
				free(graph);
			}

			inline void insert(T x, T y)
			{
				if(get(x, y)) return;

				interference_counts[hash_fn(x)]++;
				interference_counts[hash_fn(y)]++;

				graph[(size * hash_fn(y)) + hash_fn(x)] = true;
				graph[(size * hash_fn(x)) + hash_fn(y)] = true;
			}

			inline bool get(T x, T y) const
			{
				return graph[(size * hash_fn(y)) + hash_fn(x)];
			}

			void insert_key(T key)
			{
				keys[hash_fn(key)] = key;
			}
			T get_key(uint32_t index) const
			{
				return keys[index];
			}

			void get_interferences(T key, std::unordered_set<T> &interferences) const
			{
				uint32_t v = hash_fn(key);

				for(int i = 0; i < size; ++i) {
					if(graph[(size * v) + i]) interferences.insert(keys[i]);
				}
			}

			uint32_t count_interferences(T key) const
			{
				return interference_counts[hash_fn(key)];
			}

			const std::vector<T> &get_keys() const
			{
				return keys;
			}

		private:
			bool *graph;
			std::vector<T> keys;
			std::vector<uint32_t> interference_counts;

			uint32_t size;
			hash_fn_t hash_fn;
		};

	}
}



#endif /* DENSEINTERFERENCEMATRIX_H_ */
