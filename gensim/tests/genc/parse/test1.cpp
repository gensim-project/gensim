#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(GenC_Parse, BasicParseTest)
{
    // source code to test
    const std::string sourcecode = R"||(
helper void testfn()
{
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


TEST(GenC_Parse, BasicTypeParseTest)
{
    // source code to test
    const std::string sourcecode = R"||(
helper void testfn()
{
	uint8 a;
	uint16 b;
	uint32 c;
	uint64 d;
	sint8 e;
	sint16 f;
	sint32 g;
	sint64 h;
	
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
}

TEST(GenC_Parse, BasicCallParseTest)
{
    // source code to test
    const std::string sourcecode = R"||(
helper void callee()
{
	return;
}

helper void testfn()
{
	callee();
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
}

TEST(GenC_Parse, BasicOperatorParseTest)
{
    // source code to test
    const std::string sourcecode = R"||(
helper void testfn()
{
	uint32 a;
	uint32 b;

	a + b;
	a - b;
	a * b;
	a / b;
	a >> b;
	a << b;
	a >>> b;
	a <<< b;
	a | b;
	a || b;
	a & b;
	a && b;
	a ^ b;
	
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
}
