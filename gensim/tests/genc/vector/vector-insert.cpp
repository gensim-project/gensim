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

TEST(GenC_Vector, InsertConstantConstant)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8[4] v1;

	v1[0] = 0x12;
	v1[1] = 0x34;
	v1[2] = 0x56;
	v1[3] = 0x78;
	
	write_register_bank(RB, 0, v1[0]);
	write_register_bank(RB, 1, v1[1]);
	write_register_bank(RB, 2, v1[2]);
	write_register_bank(RB, 3, v1[3]);
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

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0x12);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x34);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x56);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x78);
}

TEST(GenC_Vector, InsertConstantExpression)
{
	// source code to test
	const std::string sourcecode = R"||(
helper uint8 bar(uint8 c)
{
	switch(c) {
		case 0: { return 0x12; }
		case 1: { return 0x34; }
		case 2: { return 0x56; }
		case 3: { return 0x78; }
	}
}

helper void testfn()
{
	uint8[4] v1;

	v1[0] = bar(0);
	v1[1] = bar(1);
	v1[2] = bar(2);
	v1[3] = bar(3);
	
	write_register_bank(RB, 0, v1[0]);
	write_register_bank(RB, 1, v1[1]);
	write_register_bank(RB, 2, v1[2]);
	write_register_bank(RB, 3, v1[3]);
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

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0x12);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x34);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x56);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x78);
}


TEST(GenC_Vector, InsertIncrementConstant)
{
	// source code to test
	const std::string sourcecode = R"||(

helper void testfn()
{
	uint8[4] v1;

	uint8 i = 0;

	v1[i++] = 0x12;
	v1[i++] = 0x34;
	v1[i++] = 0x56;
	v1[i++] = 0x78;
	
	write_register_bank(RB, 0, v1[0]);
	write_register_bank(RB, 1, v1[1]);
	write_register_bank(RB, 2, v1[2]);
	write_register_bank(RB, 3, v1[3]);
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

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0x12);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x34);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x56);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x78);
}


TEST(GenC_Vector, InsertIncrementExpression)
{
	// source code to test
	const std::string sourcecode = R"||(
helper uint8 bar(uint8 c)
{
	switch(c) {
		case 0: { return 0x12; }
		case 1: { return 0x34; }
		case 2: { return 0x56; }
		case 3: { return 0x78; }
	}
}

helper void testfn()
{
	uint8[4] v1;

	uint8 i = 0;

	v1[i++] = bar(0);
	v1[i++] = bar(1);
	v1[i++] = bar(2);
	v1[i++] = bar(3);
	
	write_register_bank(RB, 0, v1[0]);
	write_register_bank(RB, 1, v1[1]);
	write_register_bank(RB, 2, v1[2]);
	write_register_bank(RB, 3, v1[3]);
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

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0x12);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x34);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x56);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x78);
}
