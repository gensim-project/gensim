/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

// These are all examples which should be accepted by the parser

TEST(GenC_Parse_SwitchStmt, CaseBreak)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32 a = 0;
	switch(a) {
		case 0: {
			break;
		}
	}
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
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));
}


TEST(GenC_Parse_SwitchStmt, DefaultBreak)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32 a = 0;
	switch(a) {
		default: {
			break;
		}
	}
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
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));
}

TEST(GenC_Parse_SwitchStmt, CaseIf)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32 a = 0;
	switch(a) {
		case 0: {
			if(a) { break; } else { break; }
		}
	}
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
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));
}

TEST(GenC_Parse_SwitchStmt, DefaultIf)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint32 a = 0;
	switch(a) {
		default: {
			if(a) { break; } else { break; }
		}
	}
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
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));
}
