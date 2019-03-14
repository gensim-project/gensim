/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "wutils/vset.h"

TEST(vset, basic)
{
	wutils::vset<uint32_t> set;

	set.insert(10);
	set.insert(15);

	ASSERT_EQ(set.size(), 2);
	ASSERT_EQ(set.count(10), 1);
	ASSERT_EQ(set.count(15), 1);
}

TEST(vset, erase)
{
	wutils::vset<uint32_t> set;

	set.insert(10);
	set.insert(15);

	ASSERT_EQ(set.size(), 2);
	ASSERT_EQ(set.count(10), 1);
	ASSERT_EQ(set.count(15), 1);

	set.erase(15);

	ASSERT_EQ(set.size(), 1);
	ASSERT_EQ(set.count(10), 1);
}