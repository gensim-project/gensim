/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"
#include "genC/ssa/testing/BasicInterpreter.h"

using namespace gensim::genc::ssa::testing;

class SSA_Assembler_Assemble : public SSATestFixture {};

TEST_F(SSA_Assembler_Assemble, ReferenceParamPreserved1)
{
	const std::string ssaasm = R"||(
action void the_action ( uint8 & reference) [] < b_0 > { 
block b_0 {
s1: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "the_action");

auto the_param = test_action->GetPrototype().ParameterTypes().at(0);

ASSERT_EQ(the_param.Reference, true);
}

TEST_F(SSA_Assembler_Assemble, ReferenceParamPreserved2)
{
	const std::string ssaasm = R"||(
action void the_action ( uint8 reference) [] < b_0 > { 
block b_0 {
s1: return;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "the_action");

auto the_param = test_action->GetPrototype().ParameterTypes().at(0);

ASSERT_EQ(the_param.Reference, false);
}

TEST_F(SSA_Assembler_Assemble, ReferenceParamPreserved3)
{
	const std::string ssaasm = R"||(
action void bar ( uint8 & reference ) [] < b_0 > {
block b_0 {
	s1= constant uint8 5;
	s2: write reference s1;
	s3: return;
}
}

action uint8 the_action () [ uint8 value ] < b_0 > { 
block b_0 {
s1= constant uint8 0;
s2 : write value s1;
s3 : call bar value;
s4 = read value;
s5 : bankregwrite 0 s1 s4;
s6 : return s4;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "the_action");
ASSERT_NE(nullptr, test_action);

BasicInterpreter bi (*GetTestArch());
bi.SetRegisterState(0, 0, gensim::genc::IRConstant::Integer(0));
bool success = bi.ExecuteAction(test_action);

ASSERT_EQ(true, success);
ASSERT_EQ(5, bi.GetRegisterState(0, 0).Int());
}

TEST_F(SSA_Assembler_Assemble, ReferenceParamPassed)
{
	const std::string ssaasm = R"||(
action void baz ( uint8 & reference ) [] < b_0 > {
block b_0 {
	s1= constant uint8 5;
	s2: write reference s1;
	s3: return;
}
}
action void bar ( uint8 & reference ) [] < b_0 > {
block b_0 {
	s1: call baz reference;
	s2: return;
}
}

action uint8 the_action () [ uint8 value ] < b_0 > { 
block b_0 {
s1= constant uint8 0;
s2 : write value s1;
s3 : call bar value;
s4 = read value;
s5 : bankregwrite 0 s1 s4;
s6 : return s4;
}
}
)||";

auto test_action = CompileAsm(ssaasm, "the_action");
ASSERT_NE(nullptr, test_action);

BasicInterpreter bi (*GetTestArch());
bi.SetRegisterState(0, 0, gensim::genc::IRConstant::Integer(0));
bool success = bi.ExecuteAction(test_action);

ASSERT_EQ(true, success);
ASSERT_EQ(5, bi.GetRegisterState(0, 0).Int());
}
