/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include <cstring>
#include "wutils/wvector.h"

class allocator
{
public:
	void *realloc(void *ptr, size_t new_size)
	{
		return ::realloc(ptr, new_size);
	}
};

TEST(wvector, insert)
{
	wutils::wvector<int, allocator> v;

	ASSERT_EQ(v.size(), 0);

	v.push_back(0);

	ASSERT_EQ(v.size(), 1);

	ASSERT_EQ(v.front(), 0);
	ASSERT_EQ(v.back(), 0);

	v.push_back(1);

	ASSERT_EQ(v.size(), 2);

	ASSERT_EQ(v.front(), 0);
	ASSERT_EQ(v.back(), 1);

	v.pop_back();

	ASSERT_EQ(v.size(), 1);
	ASSERT_EQ(v.front(), 0);
	ASSERT_EQ(v.back(), 0);
}

TEST(wvector, iterate)
{
	wutils::wvector<int, allocator> v;

	v.push_back(0);
	v.push_back(1);
	v.push_back(2);
	v.push_back(3);
	v.push_back(4);

	ASSERT_EQ(v.size(), 5);

	int count = 0;
	for(auto i : v) {
		ASSERT_EQ(count, i);
		count++;
	}
}