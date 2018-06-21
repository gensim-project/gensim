/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include <vector>

#include "util/MaybeVector.h"

using namespace gensim::util;

TEST(Util_MaybeVector, InsertTest)
{
	MaybeVector<int, 2> mb;
	ASSERT_EQ(mb.size(), 0);

	mb.push_back(10);
	ASSERT_EQ(mb.at(0), 10);
	ASSERT_EQ(mb.size(), 1);

	mb.push_back(15);
	ASSERT_EQ(mb.at(0), 10);
	ASSERT_EQ(mb.at(1), 15);
	ASSERT_EQ(mb.size(), 2);

	mb.push_back(20);
	ASSERT_EQ(mb.at(0), 10);
	ASSERT_EQ(mb.at(1), 15);
	ASSERT_EQ(mb.at(2), 20);
	ASSERT_EQ(mb.size(), 3);

}

TEST(Util_MaybeVector, IterateTest)
{
	MaybeVector<int, 2> mb;

	std::vector<int> out;
	for(auto i : mb) {
		out.push_back(i);
	}
	ASSERT_EQ(mb.size(), 0);

	mb.push_back(10);
	mb.push_back(15);
	ASSERT_EQ(mb.size(), 2);

	for(auto i : mb) {
		out.push_back(i);
	}

	ASSERT_EQ(out.size(), 2);
	ASSERT_EQ(out.at(0), 10);
	ASSERT_EQ(out.at(1), 15);

	mb.push_back(20);
	ASSERT_EQ(mb.size(), 3);

	out.clear();

	for(auto i : mb) {
		out.push_back(i);
	}

	ASSERT_EQ(out.size(), 3);
	ASSERT_EQ(out.at(0), 10);
	ASSERT_EQ(out.at(1), 15);
	ASSERT_EQ(out.at(2), 20);

}

TEST(Util_MaybeVector, Count)
{
	MaybeVector<int, 2> mb;
	ASSERT_EQ(mb.size(), 0);

	mb.push_back(10);
	ASSERT_EQ(mb.size(), 1);

	mb.push_back(10);
	ASSERT_EQ(mb.size(), 2);

	mb.push_back(10);
	ASSERT_EQ(mb.size(), 3);

	mb.push_back(10);
	ASSERT_EQ(mb.size(), 4);

	mb.push_back(10);
	ASSERT_EQ(mb.size(), 5);
}

TEST(Util_MaybeVector, ConvertToVector)
{
	MaybeVector<int, 2> mb;
	std::vector<int> out0 = mb;
	ASSERT_EQ(out0.size(), 0);

	mb.push_back(10);
	mb.push_back(15);

	std::vector<int> out1 = mb;
	ASSERT_EQ(out1.size(), 2);
	ASSERT_EQ(out1.at(0), 10);
	ASSERT_EQ(out1.at(1), 15);

	mb.push_back(20);

	std::vector<int> out2 = mb;
	ASSERT_EQ(out2.size(), 3);
	ASSERT_EQ(out2.at(0), 10);
	ASSERT_EQ(out2.at(1), 15);
	ASSERT_EQ(out2.at(2), 20);
}

TEST(Util_MaybeVector, Copy)
{
	MaybeVector<int, 2> mb1;

	mb1.push_back(10);
	mb1.push_back(15);

	MaybeVector<int, 2> mb2 (mb1);
	ASSERT_EQ(mb2.size(), 2);
	ASSERT_EQ(mb2.at(0), 10);
	ASSERT_EQ(mb2.at(1), 15);

	mb1.push_back(20);
	ASSERT_EQ(mb2.size(), 2);

	mb1.push_back(25);
	auto mb3 = mb1;
	ASSERT_EQ(mb3.size(), 4);
	ASSERT_EQ(mb3.at(0), 10);
	ASSERT_EQ(mb3.at(1), 15);
	ASSERT_EQ(mb3.at(2), 20);
	ASSERT_EQ(mb3.at(3), 25);
}

TEST(Util_MaybeVector, Assignment)
{
	MaybeVector<int, 2> mb1;
	mb1.push_back(10);
	mb1.push_back(15);

	MaybeVector<int, 2> mb2;
	mb2 = mb1;

	ASSERT_EQ(mb2.size(), 2);
	ASSERT_EQ(mb2.at(0), 10);
	ASSERT_EQ(mb2.at(1), 15);

	mb1.push_back(20);
	mb2 = mb1;
	ASSERT_EQ(mb2.size(), 3);
	ASSERT_EQ(mb2.at(0), 10);
	ASSERT_EQ(mb2.at(1), 15);
	ASSERT_EQ(mb2.at(2), 20);
}

TEST(Util_MaybeVector, Clear)
{
	MaybeVector<int, 2> mb1;
	mb1.push_back(10);
	mb1.push_back(15);
	ASSERT_EQ(mb1.size(), 2);

	mb1.clear();
	ASSERT_EQ(mb1.size(), 0);

	mb1.push_back(10);
	mb1.push_back(15);
	mb1.push_back(20);
	ASSERT_EQ(mb1.size(), 3);

	mb1.clear();
	ASSERT_EQ(mb1.size(), 0);

}
