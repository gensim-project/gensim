/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
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

class SSA_Transforms_ParameterRename : public SSATestFixture { };

TEST_F(SSA_Transforms_ParameterRename, Test1)
{
	const std::string ssaasm = R"||(
	
action void subtest(uint8 & v) [] < b_0 > {
	block b_0 {
		s_0_0 = constant uint32 10;
		s_0_1 : write v s_0_0;
		s_0_2 : return;
	}
}

action void test1 () [ uint8 s ] < b_0 > { 
	block b_0 {
		s_0_0 : call subtest s;
		s_0_1 = constant uint32 0;
		s_0_2 = read s;
		s_0_3 : bankregwrite 0 s_0_1 s_0_2;
		s_0_4 : return;
	}
}
)||";

auto test_action = CompileAsm(ssaasm);
SSAPassManager manager;
manager.AddPass(SSAPassDB::Get("O3"));
manager.AddPass(SSAPassDB::Get("ParameterRename"));
manager.Run(*test_action);

BasicInterpreter bi (*GetTestArch());
bool success = bi.ExecuteAction((SSAFormAction*)test_action->GetAction("test1"));

ASSERT_EQ(true, success);
ASSERT_EQ(10, bi.GetRegisterState(0, 0).Int());
}
