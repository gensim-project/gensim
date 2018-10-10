/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include <gtest/gtest.h>

#include <cstring>
#include "wutils/vbitset.h"
#include "wutils/wvector.h"

class allocator
{
public:
	void *realloc(void *ptr, size_t new_size)
	{
		return ::realloc(ptr, new_size);
	}
};

TEST(vbitset_vector, basic)
{
	wutils::vbitset<> bitset(8);

	bitset.set(0, 1);

	ASSERT_EQ(bitset.get(0), 1);
	ASSERT_EQ(bitset.get(1), 0);

	bitset.set(0, 0);
	ASSERT_EQ(bitset.get(0), 0);

}

TEST(vbitset_wvector, basic)
{
	wutils::vbitset<wutils::wvector<bool, allocator>> bitset(8);

	bitset.set(0, 1);

	ASSERT_EQ(bitset.get(0), 1);
	ASSERT_EQ(bitset.get(1), 0);

	bitset.set(0, 0);
	ASSERT_EQ(bitset.get(0), 0);
}