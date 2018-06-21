/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/testing/BasicInterpreter.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;

using gensim::genc::IRConstant;

TEST(Issues, Issue1)
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
		default: {
			break;
		}
	}

	write_register_bank(RB, 0, c);
	return;
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

	// actually perform test
	ASSERT_EQ(true, ctx->HasAction("testfn"));
	auto action = ctx->GetAction("testfn");

	// interpret action to check output
	BasicInterpreter bi1 (ctx->GetArchDescription());
	bi1.SetRegisterState(0, 0, IRConstant::Integer(0)); // a
	bi1.SetRegisterState(0, 1, IRConstant::Integer(1)); // b
	bi1.SetRegisterState(0, 2, IRConstant::Integer(2)); // c
	bi1.SetRegisterState(0, 3, IRConstant::Integer(3)); // d
	bi1.SetRegisterState(0, 4, IRConstant::Integer(4)); // e
	bool execute_success_1 = bi1.ExecuteAction((SSAFormAction*)action);


	ASSERT_EQ(execute_success_1, true);

	uint32_t result1 = bi1.GetRegisterState(0, 0).Int();

	// optimise action
	ctx->Optimise();

	// interpret action again to check output
	BasicInterpreter bi2 (ctx->GetArchDescription());
	bi2.SetRegisterState(0, 0, IRConstant::Integer(0)); // a
	bi2.SetRegisterState(0, 1, IRConstant::Integer(1)); // b
	bi2.SetRegisterState(0, 2, IRConstant::Integer(2)); // c
	bi2.SetRegisterState(0, 3, IRConstant::Integer(3)); // d
	bi2.SetRegisterState(0, 4, IRConstant::Integer(4)); // e
	bool execute_success_2 = bi2.ExecuteAction((SSAFormAction*)action);

	ASSERT_EQ(execute_success_2, true);

	uint32_t result2 = bi2.GetRegisterState(0, 0).Int();

	ASSERT_EQ(result1, result2);
}
