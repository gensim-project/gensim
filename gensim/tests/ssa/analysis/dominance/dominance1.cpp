/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "genC/ssa/analysis/SSADominance.h"
#include "ssa/SSATestFixture.h"

class SSA_Analysis_Dominance : public SSATestFixture
{

};

gensim::genc::ssa::SSABlock *GetBlock(gensim::genc::ssa::SSAFormAction *action, std::string name)
{
	for(auto block : action->GetBlocks()) {
		if(block->GetName() == name) {
			return block;
		}
	}
	throw std::logic_error("Block not found");
}

TEST_F(SSA_Analysis_Dominance, Dominance1)
{
	const std::string ssaasm = R"||(

action void dominance_test () [] < b_0 b_1 > {

block b_0 {
	s1: jump b_1;
}

block b_1 {
	s2: return;
}

}

)||";
	
	auto action = CompileAsm(ssaasm, "dominance_test");
	
	gensim::genc::ssa::analysis::SSADominance dcalc;
	auto dominance = dcalc.Calculate(action);
	
	auto b_0 = GetBlock(action, "b_0");
	auto b_1 = GetBlock(action, "b_1");
	
	// b_1 is dominated by b_0 and b_1
	EXPECT_EQ(1U, dominance.at(b_1).count(b_0));
	EXPECT_EQ(1U, dominance.at(b_1).count(b_1));
	
	// b_0 is dominated by b_0
	EXPECT_EQ(1U, dominance.at(b_0).count(b_0));
}