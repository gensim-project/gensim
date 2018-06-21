/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "ssa/SSATestFixture.h"

using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

class SSA_Validate_RegisterIndex : public SSATestFixture
{

};

TEST_F(SSA_Validate_RegisterIndex, ReadSlotInRange)
{
	const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s1 = regread 2;
s2: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, true);
}

TEST_F(SSA_Validate_RegisterIndex, ReadSlotOutOfRange) {
	const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s1 = regread 10;
s2: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, false);
}

TEST_F(SSA_Validate_RegisterIndex, ReadBankInRange) {
		const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s0 = constant uint32 0;
s1 = bankregread 0 s0;
s2: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, true);
}

TEST_F(SSA_Validate_RegisterIndex, ReadBankOutOfRange) {
		const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s0 = constant uint32 0;
s1 = bankregread 9 s0;
s2: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, false);
}


TEST_F(SSA_Validate_RegisterIndex, WriteSlotInRange) {
	const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s0 = constant uint8 1;
s1: regwrite 2 s0;
s2: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, true);
}

TEST_F(SSA_Validate_RegisterIndex, WriteSlotOutOfRange) {
	const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s0 = constant uint8 1;
s1: regwrite 10 s0;
s2: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, false);
}

TEST_F(SSA_Validate_RegisterIndex, WriteBankInRange) {
		const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s0 = constant uint32 0;
s1 = constant uint32 10;
s2: bankregwrite 0 s0 s1;
s3: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, true);
}

TEST_F(SSA_Validate_RegisterIndex, WriteBankOutOfRange) {
		const std::string ssaasm = R"||(
action void test_action () [] < b_0 > { 
block b_0 {
s0 = constant uint32 0;
s1 = constant uint32 10;
s2: bankregwrite 9 s0 s1;
s3: return;
}
}
)||";

	bool success = TryAsmWithPass(ssaasm, GetComponent<SSAStatementValidationPass>("OperandsInSameBlockValidationPass"));
	ASSERT_EQ(success, false);
}
