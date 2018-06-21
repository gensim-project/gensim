/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(Issues, Issue2)
{
	// source code to test
	const std::string sourcecode = R"||(
execute(test_instruction){}
helper void testfn()
{
	uint32 a = read_register_bank(RB, 0);
	uint32 b = read_register_bank(RB, 1);
	uint32 c = read_register_bank(RB, 2);
	uint32 d = read_register_bank(RB, 3);
	uint32 e = read_register_bank(RB, 4);
	
	switch(a) {
		case 0: {
			if(b == 0) {
				c = d;
			} else {
				c = e;
			}
			break;
		}
	}

}
    )||";
	// parse code

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	if(ctx == nullptr) {
		std::cout << root_context;
	}

	ASSERT_NE(nullptr, ctx);

	if(ctx->Resolve(root_context)) {
		// actually perform test
		ASSERT_EQ(true, ctx->HasAction("testfn"));
		auto action = (SSAFormAction*)ctx->GetAction("testfn");

		// optimise action - this might throw an exception (and fail the test) if
		// there are any empty blocks
		ctx->Optimise();

		// check for empty blocks manually (in case optimisations become non-checking by default))
		for(auto block : action->Blocks) {
			ASSERT_NE(0, block->GetStatements().size());
		}
	}
}
