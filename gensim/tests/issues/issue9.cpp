#include <gtest/gtest.h>

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/validation/SSAActionValidator.h"

#include "ssa/SSATestFixture.h"

using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;

class Issues_Issue9 : public SSATestFixture
{

};

TEST_F(Issues_Issue9, Issue9)
{
	// source code to test
	const std::string sourcecode = R"||(
helper uint32 testfn(uint32 i)
{
	switch (i) {
	case 0: {
		i += 1;
	}
	default: {
	}
	}
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
	ASSERT_NE(nullptr, action);

	// the actual issue here is that this code should not pass validation.
	// the unreachable block elimination pass is allowed to fail on code which is not valid.
	gensim::genc::ssa::validation::SSAActionValidator validator;
	ASSERT_EQ(false, validator.Run((SSAFormAction*)action, root_context));
}
