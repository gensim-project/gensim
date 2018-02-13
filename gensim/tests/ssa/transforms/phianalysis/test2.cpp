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

TEST_F(SSA_Transforms_PhiAnalysis, ParameterTest)
{
	const std::string ssaasm = R"||(
action void test1 (uint32 param) [ ] < b_0 b_1 b_2 b_3 > { 
block b_0 {
s_0_0 = read param;
s_0_1 : if s_0_0 b_1 b_2;
}
block b_1 {
s_1_0: jump b_3;
}
block b_2 {
s_2_0 = constant uint32 0;
s_2_1: write param s_2_0;
s_2_2: jump b_3;
}
block b_3 {
s_3_0 = read param;
s_3_1 = constant uint32 0;
s_3_2: bankregwrite 0 s_3_1 s_3_0;
s_3_3: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "test1");
ASSERT_NE(test_action, nullptr);
const SSAPass *phianalysispass = SSAPassDB::Get("PhiAnalysis");
ASSERT_NE(phianalysispass, nullptr);
phianalysispass->Run(*test_action);

BasicInterpreter bi (*GetTestArch());
bool success = bi.ExecuteAction(test_action, { gensim::genc::IRConstant::Integer(0) });
ASSERT_EQ(true, success);
ASSERT_EQ(bi.GetRegisterState(0, 0).Int(), 0);

success = bi.ExecuteAction(test_action, { gensim::genc::IRConstant::Integer(1) });
ASSERT_EQ(true, success);
ASSERT_EQ(bi.GetRegisterState(0, 0).Int(), 1);

}

