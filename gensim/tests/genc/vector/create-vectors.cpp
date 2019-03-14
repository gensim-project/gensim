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

TEST(GenC_Vector, CreateSplatConstant)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8[4] v1 = 0x1f;
	
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

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0x1f);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x1f);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x1f);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x1f);
}


TEST(GenC_Vector, CreateSplatVariable)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8 v = 0x12;
	uint8[4] v1 = v;
	
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
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x12);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x12);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x12);
}


TEST(GenC_Vector, CreateVectorConstant)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8[4] v1 = { 0x12, 0x34, 0x56, 0x78 };
	
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


TEST(GenC_Vector, CreateVectorVariable)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8 s0 = 0x12;
	uint8 s1 = 0x34;
	uint8 s2 = 0x56;
	uint8 s3 = 0x78;

	uint8[4] v1 = { s0, s1, s2, s3};
	
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


TEST(GenC_Vector, CreateVectorShuffle1)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8[4] v0 = { 0x12, 0x34, 0x56, 0x78 };
	uint8[4] v1 = { v0[3],v0[2],v0[1],v0[0] };
	
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

	ASSERT_EQ(interpret.GetRegisterState(0, 0).Int(), 0x78);
	ASSERT_EQ(interpret.GetRegisterState(0, 1).Int(), 0x56);
	ASSERT_EQ(interpret.GetRegisterState(0, 2).Int(), 0x34);
	ASSERT_EQ(interpret.GetRegisterState(0, 3).Int(), 0x12);
}

TEST(GenC_Vector, CreateVectorShuffle2)
{
	// TODO: check that a vector shuffle is actually  being emitted

	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8[4] v0a = { 0x12, 0x34, 0x12, 0x34 };
	uint8[4] v0b = { 0x56, 0x78, 0x56, 0x78 };
	uint8[4] v1 = { v0a[0],v0a[1],v0b[2],v0b[3] };
	
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

TEST(GenC_Vector, CreateVectorConcatenate)
{
	// source code to test
	const std::string sourcecode = R"||(
helper void testfn()
{
	uint8[4] v0a = { 0x12, 0x34, 0x12, 0x34 };
	uint8[4] v0b = { 0x56, 0x78, 0x56, 0x78 };
	uint8[8] v1 = v0a :: v0b;
	
	write_register_bank(RB, 0, v1[0]);
	write_register_bank(RB, 1, v1[1]);
	write_register_bank(RB, 2, v1[4]);
	write_register_bank(RB, 3, v1[5]);
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
