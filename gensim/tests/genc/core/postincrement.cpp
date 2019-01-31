/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/BasicInterpreter.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(GenC_Core, BasicPostincrement)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32 a = 10;
	uint32 b = 15;

	a = b++;

	write_register_bank(RB, 0, a);
	write_register_bank(RB, 1, b);
}
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	ASSERT_NE(nullptr, ctx);

	// actually perform test
	ASSERT_EQ(true, ctx->HasAction("testfn"));

	auto action = ctx->GetAction("testfn");

	auto arch = gensim::arch::testing::GetTestArch();

	gensim::genc::ssa::testing::BasicInterpreter interpret(*arch);
	bool exec_result = interpret.ExecuteAction((gensim::genc::ssa::SSAFormAction*)action, {});
	ASSERT_EQ(true, exec_result);

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 15);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 16);
}

TEST(GenC_Core, VectorPostIncrement)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32[4] a = {10, 10, 10, 10};
	uint32 b = 0;

	a[b++] = 1;
	a[b++] = 2;
	a[b++] = 3;

	write_register_bank(RB, 0, a[0]);
	write_register_bank(RB, 1, a[1]);
	write_register_bank(RB, 2, a[2]);
	write_register_bank(RB, 3, a[3]);
	write_register_bank(RB, 4, b);
}
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	ASSERT_NE(nullptr, ctx);

	// actually perform test
	ASSERT_EQ(true, ctx->HasAction("testfn"));

	auto action = ctx->GetAction("testfn");

	auto arch = gensim::arch::testing::GetTestArch();

	gensim::genc::ssa::testing::BasicInterpreter interpret(*arch);
	bool exec_result = interpret.ExecuteAction((gensim::genc::ssa::SSAFormAction*)action, {});
	ASSERT_EQ(true, exec_result);

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 1);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 2);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 3);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 10);
	ASSERT_EQ(interpret.GetRegisterState(0, 4).Int(), 3);
}

TEST(GenC_Core, ForVectorPostIncrement)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32[4] a = {10, 10, 10, 10};
	uint32[4] c = {1, 2, 3, 10};
	uint32 b = 0;

	for(uint8 i = 0; i < 4; i++) {
		a[b++] = c[i];
	}

	write_register_bank(RB, 0, a[0]);
	write_register_bank(RB, 1, a[1]);
	write_register_bank(RB, 2, a[2]);
	write_register_bank(RB, 3, a[3]);
	write_register_bank(RB, 4, b);

	return;
}
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	ASSERT_NE(nullptr, ctx);

	// actually perform test
	ASSERT_EQ(true, ctx->HasAction("testfn"));

	auto action = ctx->GetAction("testfn");
	ctx->Optimise();

	action->Dump(std::cerr);

	auto arch = gensim::arch::testing::GetTestArch();

	gensim::genc::ssa::testing::BasicInterpreter interpret(*arch);
	bool exec_result = interpret.ExecuteAction((gensim::genc::ssa::SSAFormAction*)action, {});
	ASSERT_EQ(true, exec_result);

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 1);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 2);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 3);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 10);
	ASSERT_EQ(interpret.GetRegisterState(0, 4).Int(), 4);
}
