/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "arch/testing/TestArch.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/testing/BasicInterpreter.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

// These are all examples which should be accepted by the parser

TEST(GenC_Vector, CompareConstant)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	sint16[4] v1;
	v1[0] = 0x1234;
	v1[1] = 0x5678;
	v1[2] = 0x9abc;
	v1[3] = 0xdef0;
	
	uint8[4] out = v1 < 0;

	write_register_bank(RB, 0, out[0]);
	write_register_bank(RB, 1, out[1]);
	write_register_bank(RB, 2, out[2]);
	write_register_bank(RB, 3, out[3]);
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
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));

	auto action = ctx->GetAction("testfn");

	auto arch = gensim::arch::testing::GetTestArch();

	gensim::genc::ssa::testing::BasicInterpreter interpret(*arch);
	bool exec_result = interpret.ExecuteAction((gensim::genc::ssa::SSAFormAction*)action, {});
	ASSERT_EQ(true, exec_result);

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0xff);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0xff);
}
