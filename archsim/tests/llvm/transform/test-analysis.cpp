/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifdef ARCHSIM_ENABLE_LLVM

#include <gtest/gtest.h>

#include "translate/llvm/LLVMRegisterOptimisationPass.h"

using namespace archsim::translate::translate_llvm;

TEST(Archsim_LLVM_Analysis_RegisterDefinition, Merge1)
{
	RegisterAccessDB radb;
	RegisterDefinitions rd1 (radb), rd2 (radb);

	auto byte0_access = radb.Insert(RegisterAccess::Load(nullptr, RegisterReference({0,0}, true)));
	auto byte1_access = radb.Insert(RegisterAccess::Load(nullptr, RegisterReference({1,1}, true)));

	rd1.AddDefinition(&radb.Get(byte0_access));
	rd2.AddDefinition(&radb.Get(byte1_access));

	RegisterDefinitions merged = RegisterDefinitions::MergeDefinitions({&rd1, &rd2}, radb);

	auto byte0 = merged.GetDefinitions({0,0});
	auto byte1 = merged.GetDefinitions({1,1});
	auto others = merged.GetDefinitions({2,INT_MAX});

	ASSERT_EQ(byte0.size(), 1);
	ASSERT_EQ(byte0.count(byte0_access), 1);
	ASSERT_EQ(byte1.size(), 1);
	ASSERT_EQ(byte1.count(byte1_access), 1);
	ASSERT_EQ(others.size(), 0);
}


TEST(Archsim_LLVM_Analysis_RegisterDefinition, Merge2)
{
	RegisterAccessDB radb;
	RegisterDefinitions rd1 (radb), rd2 (radb);

	auto byte0_access = radb.Insert(RegisterAccess::Load(nullptr, RegisterReference({0,0}, true)));
	auto byte1_access = radb.Insert(RegisterAccess::Load(nullptr, RegisterReference({0,0}, true)));

	rd1.AddDefinition(&radb.Get(byte0_access));
	rd2.AddDefinition(&radb.Get(byte1_access));

	RegisterDefinitions merged = RegisterDefinitions::MergeDefinitions({&rd1, &rd2}, radb);

	auto byte0 = merged.GetDefinitions({0,0});
	auto others = merged.GetDefinitions({1,INT_MAX});

	ASSERT_EQ(byte0.size(), 2);
	ASSERT_EQ(byte0.count(byte0_access), 1);
	ASSERT_EQ(byte0.count(byte1_access), 1);
	ASSERT_EQ(others.size(), 0);
}

TEST(Archsim_LLVM_Analysis_RegisterDefinition, Merge3)
{
	RegisterAccessDB radb;
	RegisterDefinitions rd1 (radb), rd2 (radb);

	auto byte0_access = radb.Insert(RegisterAccess::Load(nullptr, RegisterReference({0,1}, true)));
	auto byte1_access = radb.Insert(RegisterAccess::Load(nullptr, RegisterReference({1,2}, true)));

	rd1.AddDefinition(&radb.Get(byte0_access));
	rd2.AddDefinition(&radb.Get(byte1_access));

	RegisterDefinitions merged = RegisterDefinitions::MergeDefinitions({&rd1, &rd2}, radb);

	auto byte0 = merged.GetDefinitions({0,0});
	auto byte1 = merged.GetDefinitions({1,1});
	auto byte2 = merged.GetDefinitions({2,2});
	auto others = merged.GetDefinitions({3,INT_MAX});

	ASSERT_EQ(byte0.size(), 1);
	ASSERT_EQ(byte0.count(byte0_access), 1);

	ASSERT_EQ(byte1.size(), 2);
	ASSERT_EQ(byte1.count(byte0_access), 1);
	ASSERT_EQ(byte1.count(byte1_access), 1);

	ASSERT_EQ(byte2.size(), 1);
	ASSERT_EQ(byte2.count(byte1_access), 1);

	ASSERT_EQ(others.size(), 0);
}

TEST(Archsim_LLVM_Analysis_RegisterDefinition, Definitions1)
{
	RegisterAccessDB radb;

	auto store0to3 = radb.Insert(RegisterAccess::Store(nullptr, RegisterReference({0, 3},true)));
	auto store0to0 = radb.Insert(RegisterAccess::Store(nullptr, RegisterReference({0, 0},true)));

	BlockInformation bi (radb);
	bi.GetAccesses().push_back(store0to3);
	bi.GetAccesses().push_back(store0to0);

	BlockDefinitions bd (bi, radb);

	RegisterDefinitions rd(radb);
	std::vector<uint64_t> live;
	rd = bd.PropagateDefinitions(rd, live);

	auto defs0 = rd.GetDefinitions({0,0});
	ASSERT_EQ(defs0.size(), 1);
	ASSERT_EQ(defs0.count(store0to0), 1);

	auto defs1to3 = rd.GetDefinitions({1,3});
	ASSERT_EQ(defs1to3.size(), 1);
	ASSERT_EQ(defs1to3.count(store0to3), 1);

	auto defs0to3 = rd.GetDefinitions({0,3});
	ASSERT_EQ(defs0to3.size(), 2);
	ASSERT_EQ(defs0to3.count(store0to3), 1);
	ASSERT_EQ(defs0to3.count(store0to0), 1);
}

#endif