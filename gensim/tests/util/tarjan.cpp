/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include <vector>

#include "util/Tarjan.h"

TEST(Util_Tarjan, Test1)
{
	std::vector<int> nodes { 1, 2, 3, 4, 5 };
	std::vector<std::pair<int, int>> edges {}; // no edges

	gensim::util::Tarjan<int> tarjan (nodes.begin(), nodes.end(), edges.begin(), edges.end());
	auto sccs = tarjan.Compute();

	ASSERT_EQ(sccs.size(), 5);
}

TEST(Util_Tarjan, Test2)
{
	std::vector<int> nodes { 1, 2, 3, 4, 5 };
	std::vector<std::pair<int, int>> edges {{1, 2}, {2,1}}; // two edges

	gensim::util::Tarjan<int> tarjan (nodes.begin(), nodes.end(), edges.begin(), edges.end());
	auto sccs = tarjan.Compute();

	ASSERT_EQ(sccs.size(), 4);
}

TEST(Util_Tarjan, Test3)
{
	std::vector<int> nodes { 1, 2, 3, 4, 5 };
	std::vector<std::pair<int, int>> edges {{1, 2}, {2,1}, {3,4}, {4,3}, {4, 5}, {5, 4}}; // two edges

	gensim::util::Tarjan<int> tarjan (nodes.begin(), nodes.end(), edges.begin(), edges.end());
	auto sccs = tarjan.Compute();

	ASSERT_EQ(sccs.size(), 2);
}
