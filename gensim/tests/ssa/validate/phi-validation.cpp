#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/validation/SSAStatementValidationPass.h"

using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa::validation;
using namespace gensim::genc::ssa;

class SSA_Validate_PhiNodeValidation : public SSATestFixture
{
public:
	bool Validate(SSAFormAction *action)
	{
		SSAActionValidationPass *pass = GetComponent<SSAActionValidationPass>("PhiNodeValidationPass");

		gensim::DiagnosticSource src ("test");
		gensim::DiagnosticContext ctx (src);

		return pass->Run(action, ctx);
	}
};

TEST_F(SSA_Validate_PhiNodeValidation, MemberTypesOK)
{
	const std::string ssa_asm = R"||(

action void test_action () [] < b_0 b_1 b_2 b_3 > { 
block b_0 {
s0 = constant uint32 0;
s1: if s0 b_1 b_2;
}

block b_1 {
	s1_0 = constant uint32 0;
	s1_1 : jump b_3;
}

block b_2 {
	s2_0 = constant uint32 1;
	s2_1 : jump b_3;
}

block b_3 {
	s3_0 = phi uint32 [ s1_0 s2_0 ];
	s3_1: return;
}

}	
)||";

	auto action = CompileAsm(ssa_asm, "test_action");
	if(action == nullptr) {
		std::cerr << Diag();
	}
	ASSERT_NE(action, nullptr);
	bool validates = Validate(action);
	ASSERT_EQ(validates, true);
}
