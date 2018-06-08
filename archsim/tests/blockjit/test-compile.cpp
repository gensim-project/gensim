#include <gtest/gtest.h>

#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/IRInstruction.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"

using namespace captive::arch::jit;
using namespace captive::shared;

archsim::ArchDescriptor GetArch() {
	archsim::ISABehavioursDescriptor behaviours({});
	archsim::ISADescriptor isa("isa", 0, [](archsim::Address addr, archsim::MemoryInterface *, gensim::BaseDecode&){ UNIMPLEMENTED; return 0u; }, []()->gensim::BaseDecode*{ UNIMPLEMENTED; }, []()->gensim::BaseJumpInfo*{ UNIMPLEMENTED; }, []()->gensim::DecodeTranslateContext*{ UNIMPLEMENTED; }, behaviours);
	archsim::FeaturesDescriptor f({});
	archsim::MemoryInterfacesDescriptor mem({}, "");
	archsim::RegisterFileDescriptor rf(128, {});
	archsim::ArchDescriptor arch ("test_arch", rf, mem, f, {isa});
	
	return arch;
}

class ArchSimBlockJITTest : public ::testing::Test {
public:
	ArchSimBlockJITTest() : Arch(GetArch()) {}
	
	archsim::ArchDescriptor Arch;
	archsim::StateBlockDescriptor StateBlockDescriptor;
	wulib::SimpleZoneMemAllocator Allocator;
	
	block_txln_fn CompileAndLower(TranslationContext &tc) {
		BlockCompiler block_compiler (tc, 0, Allocator);
	
		auto result = block_compiler.compile(false);

		return Lower(tc, result);
	}
	
	block_txln_fn Lower(TranslationContext &tc, const CompileResult &cr) {
		return captive::arch::jit::lowering::NativeLowering(tc, Allocator, Arch, StateBlockDescriptor, cr).Function;
	}
};

TEST_F(ArchSimBlockJITTest, TestCompile1) 
{	
	
	TranslationContext tc;
	tc.alloc_block();
	tc.add_instruction(0, captive::shared::IRInstruction::ret());
	
	auto fn = CompileAndLower(tc);
	ASSERT_NE(nullptr, fn);
	fn(nullptr, nullptr);
}

TEST_F(ArchSimBlockJITTest, WriteReg)
{
	using namespace captive::shared;

	TranslationContext tc;
	tc.alloc_block();
	tc.add_instruction(0, captive::shared::IRInstruction::streg(IROperand::const32(0xffffffff), IROperand::const32(0)));
	tc.add_instruction(0, captive::shared::IRInstruction::ret());
	
	auto fn = CompileAndLower(tc);
	ASSERT_NE(nullptr, fn);
	
	std::vector<char> regfile_mock(128, 0);
	fn(regfile_mock.data(), nullptr);
	
	ASSERT_EQ(*(uint32_t*)regfile_mock.data(), 0xffffffff);
}

TEST_F(ArchSimBlockJITTest, WriteRegFromStack)
{
	using namespace captive::shared;

	TranslationContext tc;
	tc.alloc_block();
	
	IROperand written_value = IROperand::vreg(tc.alloc_reg(4), 4);
	written_value.allocate(IROperand::ALLOCATED_STACK, 0);
	
	tc.add_instruction(0, captive::shared::IRInstruction::mov(IROperand::const32(0xffffffff), written_value));
	tc.add_instruction(0, captive::shared::IRInstruction::streg(written_value, IROperand::const32(0)));
	tc.add_instruction(0, captive::shared::IRInstruction::ret());
	
	CompileResult cr(true, 8, archsim::util::vbitset(8, 0xff));

	auto fn = Lower(tc, cr);
	ASSERT_NE(nullptr, fn);
	
	std::vector<char> regfile_mock(128, 0);
	fn(regfile_mock.data(), nullptr);
	
	ASSERT_EQ(*(uint32_t*)regfile_mock.data(), 0xffffffff);
}