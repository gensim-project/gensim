/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
