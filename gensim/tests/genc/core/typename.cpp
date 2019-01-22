/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(GenC_Parse, Typename1)
{
	// source code to test
	const std::string sourcecode = R"||(

typename test_type = uint32;

helper void testfn(struct Operand &inst)
{
	return;
}
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

TEST(GenC_Parse, Typename2)
{
	// source code to test
	const std::string sourcecode = R"||(

typename test_type = uint32;

helper void testfn()
{
	typename test_type variable = 10;
	return;
}
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

TEST(GenC_Parse, TypenameDuplicate1)
{
	// source code to test
	const std::string sourcecode = R"||(

typename test_type = uint32;
typename test_type = uint32;

helper void testfn()
{
	typename test_type variable = 10;
	return;
}
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	std::cerr << root_context;

	// Should fail to parse
	ASSERT_EQ(nullptr, ctx);
}

TEST(GenC_Parse, TypenameDuplicate2)
{
	// source code to test
	const std::string sourcecode = R"||(

typename uint8 = uint32;

helper void testfn()
{
	typename test_type variable = 10;
	return;
}
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	std::cerr << root_context;

	// Should fail to parse
	ASSERT_EQ(nullptr, ctx);
}