/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"
#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/statement/SSAPhiStatement.h"


using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;

class SSA_Transforms_PhiElimination : public SSATestFixture { };

TEST_F(SSA_Transforms_PhiElimination, Test1)
{
	const std::string ssaasm = R"||(
action void noloop () [] < b_0 b_1 b_2 > { 
block b_0 {
s1 = constant uint32 10;
s2: if s1 b_1 b_2;
}
block b_1 {
s_1_0 = phi uint32 [ s1 ];
s_1_1: regwrite 1 s_1_0;
s_1_2: return;
}
block b_2 {
s_2_0 = phi uint32 [ s1 ];
s_2_1: regwrite 1 s_2_0;
s_2_2: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "noloop");

const SSAPass *phieliminationpass = SSAPassDB::Get("PhiElimination");
ASSERT_NE(nullptr, phieliminationpass);

while(phieliminationpass->Run(*test_action));

// there should be only one symbol since both phi nodes should take the same phi symbol
ASSERT_EQ(1, test_action->Symbols().size());

}
