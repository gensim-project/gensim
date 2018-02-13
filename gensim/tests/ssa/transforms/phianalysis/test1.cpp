#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"
#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/testing/BasicInterpreter.h"


using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;

class SSA_Transforms_PhiAnalysis : public SSATestFixture { };

TEST_F(SSA_Transforms_PhiAnalysis, Test1)
{
	const std::string ssaasm = R"||(
action void test1 () [ uint32 s ] < b_0 b_1 b_2 > { 
block b_0 {
s1 = constant uint32 10;
s2: write s s1;
s3: if s1 b_1 b_2;
}
block b_1 {
s_1_0 = constant uint32 5;
s_1_1: write s s_1_0;
s_1_2: jump b_2;
}
block b_2 {
s_2_0 = read s;
s_2_1 = constant uint32 0;
s_2_2: bankregwrite 0 s_2_1 s_2_0;
s_2_3: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "test1");
const SSAPass *phianalysispass = SSAPassDB::Get("PhiAnalysis");
ASSERT_NE(phianalysispass, nullptr);
phianalysispass->Run(*test_action);

BasicInterpreter bi (*GetTestArch());
bool success = bi.ExecuteAction(test_action);

ASSERT_EQ(true, success);
ASSERT_EQ(5, bi.GetRegisterState(0, 0).Int());
}

TEST_F(SSA_Transforms_PhiAnalysis, Test2)
{
	const std::string ssaasm = R"||(
action void test1 () [ uint32 s ] < b_0 b_1 b_2 b_3 b_4 > { 
block b_0 {
s1 = constant uint32 0;
s2: write s s1;
s3: if s1 b_1 b_2;
}
block b_1 {
s_1_0 = read s;
s_1_1 = constant uint32 5;
s_1_2 = binary + s_1_0 s_1_1;
s_1_3: write s s_1_2;
s_1_4: jump b_2;
}
block b_2 {
s_2_0 = constant uint32 1;
s_2_1: if s_2_0 b_3 b_4;
}

block b_3 {
s_3_0 = read s;
s_3_1 = constant uint32 3;
s_3_2 = binary + s_3_0 s_3_1;
s_3_3: write s s_3_2;
s_3_4: jump b_4;
}

block b_4 {
s_4_0 = read s;
s_4_1 = constant uint32 0;
s_4_2: bankregwrite 0 s_4_1 s_4_0;
s_4_3: return;
}

}
)||";

auto test_action = CompileAsm(ssaasm, "test1");
ASSERT_NE(test_action, nullptr);

const SSAPass *phianalysispass = SSAPassDB::Get("PhiAnalysis");
ASSERT_NE(phianalysispass, nullptr);

phianalysispass->Run(*test_action);

BasicInterpreter bi (*GetTestArch());
bool success = bi.ExecuteAction(test_action);

ASSERT_EQ(true, success);
ASSERT_EQ(3, bi.GetRegisterState(0, 0).Int());
}


TEST_F(SSA_Transforms_PhiAnalysis, Test3)
{
	const std::string ssaasm = R"||(
action void test1 () [ uint32 s ] < b_0 b_1 b_2 > { 
block b_0 {
s1 = constant uint32 1;
s2: write s s1;
s3: if s1 b_1 b_2;
}
block b_1 {
s_1_0 = constant uint32 5;
s_1_1: write s s_1_0;
s_1_2 = read s;
s_1_3 = constant uint32 1;
s_1_4: bankregwrite 0 s_1_3 s_1_2;
s_1_5 = constant uint32 3;
s_1_6: write s s_1_5;
s_1_7: jump b_2;
}
block b_2 {
s_2_0 = read s;
s_2_1 = constant uint32 0;
s_2_2: bankregwrite 0 s_2_1 s_2_0;
s_2_3: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "test1");
const SSAPass *phianalysispass = SSAPassDB::Get("PhiAnalysis");
ASSERT_NE(phianalysispass, nullptr);
phianalysispass->Run(*test_action);

BasicInterpreter bi (*GetTestArch());
bool success = bi.ExecuteAction(test_action);

ASSERT_EQ(true, success);
ASSERT_EQ(3, bi.GetRegisterState(0, 0).Int());
ASSERT_EQ(5, bi.GetRegisterState(0, 1).Int());
}
