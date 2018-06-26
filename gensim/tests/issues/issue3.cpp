/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/statement/SSAReturnStatement.h"
#include "genC/ssa/SSABlock.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(Issues, Issue3)
{
	// source code to test
	const std::string sourcecode = R"||(
execute(test_instruction){}
helper void testfn()
{

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
	auto action = (SSAFormAction*)ctx->GetAction("testfn");

	// testfn must contain a return
	bool found_return = false;
	for(auto block : action->GetBlocks()) {
		for(auto statement : block->GetStatements()) {
			if(dynamic_cast<SSAReturnStatement*>(statement)) {
				found_return = true;
			}
		}
	}

	ASSERT_EQ(true, found_return);
}
