/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"
#include "genC/ssa/testing/BasicInterpreter.h"

using namespace gensim::genc::ssa::testing;

class SSA_Functional_PassStruct : public SSATestFixture {};

TEST_F(SSA_Functional_PassStruct, BasicInstructionPass)
{
	const std::string ssaasm = R"||(
action void callee (struct Instruction & reference) [] < b_0 > { 
block b_0 {
s1: return;
}
}

action void caller (struct Instruction & reference) [] < b_0 > { 
block b_0 {
s1: call callee reference;
s2: return;
}
}

)||";
	
	auto action = CompileAsm(ssaasm, "caller");
	if(action == nullptr) {
		std::cerr << Diag();
		FAIL();
	}
}