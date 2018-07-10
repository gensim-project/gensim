/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(GenC_Parse, PassStruct)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn(Instruction &inst)
{
	return;
}

execute(test_insn) { testfn(inst); }
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	std::cerr << root_context;

	ASSERT_NE(nullptr, ctx);

	// actually perform test
	ASSERT_EQ(true, ctx->HasAction("testfn"));
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));
}

TEST(GenC_Parse, PassStructValue)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn(Instruction inst)
{
	return;
}

execute(test_insn) { testfn(inst); }
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	std::cerr << root_context;

	// should fail to parse
	ASSERT_EQ(nullptr, ctx);
}
