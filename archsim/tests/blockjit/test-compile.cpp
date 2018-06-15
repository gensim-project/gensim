#include <gtest/gtest.h>

#include "inc/ArchSimBlockJITTest.h"

#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/IRBuilder.h"
#include "blockjit/block-compiler/transforms/Transform.h"
#include "blockjit/IRInstruction.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"

using namespace captive::arch::jit;
using namespace captive::shared;

TEST_F(ArchSimBlockJITTest, TestCompile1) 
{	
	Builder().ret();
	
	auto fn = CompileAndLower();
	ASSERT_NE(nullptr, fn);
	fn(nullptr, nullptr);
}

TEST_F(ArchSimBlockJITTest, WriteReg)
{
	using namespace captive::shared;

	Builder().streg(IROperand::const32(0xffffffff), IROperand::const32(0));
	Builder().ret();
	
	auto fn = CompileAndLower();
	ASSERT_NE(nullptr, fn);
	
	std::vector<char> regfile_mock(128, 0);
	fn(regfile_mock.data(), nullptr);
	
	ASSERT_EQ(*(uint32_t*)regfile_mock.data(), 0xffffffff);
}

TEST_F(ArchSimBlockJITTest, WriteRegFromStack)
{
	using namespace captive::shared;

	IROperand written_value = IROperand::vreg(tc_.alloc_reg(4), 4);
	written_value.allocate(IROperand::ALLOCATED_STACK, 0);
	
	Builder().mov(IROperand::const32(0xffffffff), written_value);
	Builder().streg(written_value, IROperand::const32(0));
	Builder().ret();
	
	CompileResult cr(true, 8, archsim::util::vbitset(8, 0xff));

	auto fn = Lower(cr);
	ASSERT_NE(nullptr, fn);
	
	std::vector<char> regfile_mock(128, 0);
	fn(regfile_mock.data(), nullptr);
	
	ASSERT_EQ(*(uint32_t*)regfile_mock.data(), 0xffffffff);
}
