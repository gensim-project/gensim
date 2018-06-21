/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/analysis/LoopAnalysis.h"


using namespace gensim::genc::ssa::testing;

class SSA_Analysis_LoopAnalysis : public SSATestFixture
{

};

TEST_F(SSA_Analysis_LoopAnalysis, NoLoopDetected)
{
	const std::string ssaasm = R"||(
action void noloop () [] < b_0 b_1 b_2 > { 
block b_0 {
s1: jump b_1;
}
block b_1 {
s2: jump b_2;
}
block b_2 {
s3: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "noloop");

gensim::genc::ssa::LoopAnalysis la;
auto result = la.Analyse(*test_action);

ASSERT_EQ(result.LoopExists, false);
}


TEST_F(SSA_Analysis_LoopAnalysis, LoopDetected)
{
const std::string ssaasm = R"||(
action void noloop () [] < b_0 b_1 b_2 > { 
block b_0 {
s1: jump b_1;
}
block b_1 {
s2: jump b_0;
}
block b_2 {
s3: return;
}
}
)||";
auto test_action = CompileAsm(ssaasm, "noloop");

gensim::genc::ssa::LoopAnalysis la;
auto result = la.Analyse(*test_action);

ASSERT_EQ(result.LoopExists, true);
}
