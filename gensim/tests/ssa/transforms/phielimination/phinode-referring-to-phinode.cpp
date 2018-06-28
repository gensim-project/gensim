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

class SSA_Transforms_PhiElimination : public SSATestFixture { };

TEST_F(SSA_Transforms_PhiElimination, PhiNodeReferToPhiNode)
{
	const std::string ssaasm = R"||(
action void noloop () [] < b_0 b_1 b_2 b_3 b_4 b_5 > { 
	block b_0 {
		s_0_0 = constant uint32 10;
		s_0_1: if s_0_0 b_1 b_5;
	}
	block b_1 {
		s_1_0 = constant uint32 5;
		s_1_1: if s_1_0 b_2 b_3;
	}
	block b_2 {
		s_2_0 = constant uint32 8;
		s_2_1: jump b_4;
	}
	block b_3 {
		s_3_0 = constant uint32 9;
		s_3_1: jump b_4;
	}
	block b_4 {
		s_4_0 = phi uint32 [ s_2_0 s_3_0 ];
		s_4_1: jump b_5;
	}
	block b_5 {
		s_5_0 = phi uint32 [ s_0_0 s_4_0 ];
		s_5_1 = constant uint32 0;
		s_5_2: bankregwrite 0 s_5_1 s_5_0;
		s_5_3: return;
	}
}
)||";

auto test_action = CompileAsm(ssaasm, "noloop");
ASSERT_NE(nullptr, test_action);

const SSAPass *phieliminationpass = SSAPassDB::Get("PhiSetElimination");
ASSERT_NE(nullptr, phieliminationpass);

BasicInterpreter bi (*GetTestArch());

bi.ExecuteAction(test_action);
uint32_t pre_value = bi.GetRegisterState(0, 0).Int();

phieliminationpass->Run(*test_action);

bi.ExecuteAction(test_action);
uint32_t post_value = bi.GetRegisterState(0, 0).Int();

ASSERT_EQ(pre_value, post_value);
}
