/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include <gtest/gtest.h>

#include <cstring>
#include "wutils/wlist.h"

class allocator
{
public:
	void *realloc(void *ptr, size_t new_size)
	{
		return ::realloc(ptr, new_size);
	}
};

TEST(wlist, basic)
{
	wutils::wlist<int> w;
	w.push_front(0);

	ASSERT_EQ(w.size(), 1);
	ASSERT_EQ(w.front(), 0);
}

TEST(wlist, iterate)
{
	wutils::wlist<int> w;

	w.push_front(4);
	w.push_front(3);
	w.push_front(2);
	w.push_front(1);
	w.push_front(0);

	int count = 0;
	for(auto i : w) {
		ASSERT_EQ(count, i);
		count++;
	}
}
