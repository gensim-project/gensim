/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "translate/llvm/LLVMRegisterOptimisationPass.h"

using namespace archsim::translate::translate_llvm;

TEST(Archsim_LLVM_Analysis_RegisterDefinition, Merge1)
{
	RegisterDefinitions rd1, rd2;
	TagContext tc;

	auto byte0_access = RegisterAccess::Load(nullptr, RegisterReference({0,0}, true), tc.NextTag());
	auto byte1_access = RegisterAccess::Load(nullptr, RegisterReference({1,1}, true), tc.NextTag());

	rd1.AddDefinition(&byte0_access);
	rd2.AddDefinition(&byte1_access);

	RegisterDefinitions merged = RegisterDefinitions::MergeDefinitions({&rd1, &rd2});

	auto byte0 = merged.GetDefinitions({0,0});
	auto byte1 = merged.GetDefinitions({1,1});
	auto others = merged.GetDefinitions({2,INT_MAX});

	ASSERT_EQ(byte0.size(), 1);
	ASSERT_EQ(byte0.count(&byte0_access), 1);
	ASSERT_EQ(byte1.size(), 1);
	ASSERT_EQ(byte1.count(&byte1_access), 1);
	ASSERT_EQ(others.size(), 0);
}


TEST(Archsim_LLVM_Analysis_RegisterDefinition, Merge2)
{
	RegisterDefinitions rd1, rd2;
	TagContext tc;

	auto byte0_access = RegisterAccess::Load(nullptr, RegisterReference({0,0}, true), tc.NextTag());
	auto byte1_access = RegisterAccess::Load(nullptr, RegisterReference({0,0}, true), tc.NextTag());

	rd1.AddDefinition(&byte0_access);
	rd2.AddDefinition(&byte1_access);

	RegisterDefinitions merged = RegisterDefinitions::MergeDefinitions({&rd1, &rd2});

	auto byte0 = merged.GetDefinitions({0,0});
	auto others = merged.GetDefinitions({1,INT_MAX});

	ASSERT_EQ(byte0.size(), 2);
	ASSERT_EQ(byte0.count(&byte0_access), 1);
	ASSERT_EQ(byte0.count(&byte1_access), 1);
	ASSERT_EQ(others.size(), 0);
}

TEST(Archsim_LLVM_Analysis_RegisterDefinition, Merge3)
{
	RegisterDefinitions rd1, rd2;
	TagContext tc;

	auto byte0_access = RegisterAccess::Load(nullptr, RegisterReference({0,1}, true), tc.NextTag());
	auto byte1_access = RegisterAccess::Load(nullptr, RegisterReference({1,2}, true), tc.NextTag());

	rd1.AddDefinition(&byte0_access);
	rd2.AddDefinition(&byte1_access);

	RegisterDefinitions merged = RegisterDefinitions::MergeDefinitions({&rd1, &rd2});

	auto byte0 = merged.GetDefinitions({0,0});
	auto byte1 = merged.GetDefinitions({1,1});
	auto byte2 = merged.GetDefinitions({2,2});
	auto others = merged.GetDefinitions({3,INT_MAX});

	ASSERT_EQ(byte0.size(), 1);
	ASSERT_EQ(byte0.count(&byte0_access), 1);

	ASSERT_EQ(byte1.size(), 2);
	ASSERT_EQ(byte1.count(&byte0_access), 1);
	ASSERT_EQ(byte1.count(&byte1_access), 1);

	ASSERT_EQ(byte2.size(), 1);
	ASSERT_EQ(byte2.count(&byte1_access), 1);

	ASSERT_EQ(others.size(), 0);
}