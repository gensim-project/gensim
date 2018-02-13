//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================

#ifndef __HEADER_HASHMAP
#define __HEADER_HASHMAP

#include <cassert>
#include <map>
#include <list>
#include <vector>

namespace archsim
{
	namespace util
	{

		template <typename Key, typename Val, int Size = 4>
		class HashMapBucket
		{
		private:
			typedef std::pair<Key, Val> BucketEntry;
			typedef std::vector<BucketEntry> BucketType;
			typedef typename BucketType::iterator BucketIterator;

			BucketType bucket;
			uint8_t filled_entries;

		public:
			uint32_t collisions;
			HashMapBucket() : filled_entries(0)
			{
				bucket.reserve(Size);
			}

			BucketIterator begin()
			{
				return bucket.begin();
			}
			BucketIterator end()
			{
				return bucket.end();
			}

			void insert(Key k, Val v)
			{
				assert(filled_entries < Size);
				bucket.push_back(std::make_pair(k, v));
			}

			Val &get(Key k)
			{
				for (BucketIterator i = begin(); i != end(); ++i) {
					if (i->first == k) return i->second;
				}
				assert(false);
			}

			Val *getPtr(Key k)
			{
				for (BucketIterator i = begin(); i != end(); ++i) {
					if (i->first == k) return &i->second;
					collisions++;
				}
				return NULL;
			}

			bool contains(Key k)
			{
				for (BucketIterator i = begin(); i != end(); ++i) {
					if (i->first == k) return true;
				}
				return false;
			}

			uint32_t load()
			{
				return bucket.size();
			}
		};

		template <typename Key, typename Val, int bucket_count = 256>
		class HashMap
		{
		public:
			typedef uint32_t (*HashType)(Key k);

		private:
			HashMapBucket<Key, Val> buckets[bucket_count];
			HashType hash_fn;

		public:
			HashMap(HashType h) : hash_fn(h) {};

			void insert(Key k, Val v)
			{
				uint32_t h = hash_fn(k);
				h %= bucket_count;
				buckets[h].insert(k, v);
			}

			Val &get(Key k)
			{
				uint32_t h = hash_fn(k);
				h %= bucket_count;
				return buckets[h].get(k);
			}

			Val *getPtr(Key k)
			{
				uint32_t h = hash_fn(k);
				h %= bucket_count;
				return buckets[h].getPtr(k);
			}

			bool contains(Key k)
			{
				uint32_t h = hash_fn(k);
				h %= bucket_count;
				return buckets[h].contains(k);
			}

			uint32_t load_average_squared()
			{
				uint32_t total = 0;
				for (int i = 0; i < bucket_count; ++i) {
					total += buckets[i].load() * buckets[i].load();
				}
				return total;
			}

			uint32_t collisions()
			{
				uint32_t total = 0;
				for (int i = 0; i < bucket_count; ++i) {
					total += buckets[i].collisions;
				}
				return total;
			}
		};
	}
}

#endif
