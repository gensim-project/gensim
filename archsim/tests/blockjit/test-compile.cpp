#include <gtest/gtest.h>

#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/IRBuilder.h"
#include "blockjit/block-compiler/transforms/Transform.h"
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
	
	block_txln_fn CompileAndLower() {
		BlockCompiler block_compiler (tc_, 0, Allocator);
	
		auto result = block_compiler.compile(false);

		return Lower(result);
	}
	
	block_txln_fn Lower(const CompileResult &cr) {
		return captive::arch::jit::lowering::NativeLowering(tc_, Allocator, Arch, StateBlockDescriptor, cr).Function;
	}
	
	IRRegId StackValue(uint64_t value, uint8_t size) {
		IRRegId reg = tc_.alloc_reg(size);
		allocations_[reg] = std::make_pair(IROperand::ALLOCATED_STACK, stack_frame_);
		stack_frame_ += 8;
		
		Builder().mov(IROperand::constant(value, size), IROperand::vreg(reg, size));
		return reg;
	}
	
	IRRegId RegValue(uint64_t value, uint8_t size, uint8_t reg_id) {
		IRRegId reg = tc_.alloc_reg(size);
		allocations_[reg] = std::make_pair(IROperand::ALLOCATED_REG, reg_id);
		
		builder_.mov(IROperand::constant(value, size), IROperand::vreg(reg, size));
		return reg;
	}
	
	void SetUp() override {
		tc_.clear();
		stack_frame_ = 0;
		allocations_.clear();
		builder_.SetContext(&tc_);
		builder_.SetBlock(tc_.alloc_block());
	}

	IRBuilder &Builder() { return builder_; }
	
	captive::arch::jit::transforms::AllocationWriterTransform::allocations_t allocations_;
	uint32_t stack_frame_;
	TranslationContext tc_;
	IRBuilder builder_;
};

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

class ArchSimBlockJITCmpTest : public ArchSimBlockJITTest {
public:
	
	void build_cmp_skeleton_sss(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_STACK, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_STACK, 8);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_STACK, 16);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_rss(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_STACK, 8);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_STACK, 16);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_srs(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_STACK, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_STACK, 16);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_ssr(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_STACK, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_STACK, 8);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_rrs(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_REG, 1);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_STACK, 0);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_rsr(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_STACK, 0);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_REG, 1);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_srr(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_STACK, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_REG, 1);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_cmp_skeleton_rrr(std::function<IRInstruction(IRRegId lh, IRRegId rh, IRRegId output)> cmp_factory) {
		using namespace captive::shared;
	
		IRRegId lh = Builder().alloc_reg(4);
		IRRegId rh = Builder().alloc_reg(4);
		IRRegId output = Builder().alloc_reg(1);

		allocations_[lh] = std::make_pair(IROperand::ALLOCATED_REG, 0);
		allocations_[rh] = std::make_pair(IROperand::ALLOCATED_REG, 1);
		allocations_[output] = std::make_pair(IROperand::ALLOCATED_REG, 2);
		stack_frame_ = 16;

		Builder().ldreg(IROperand::const32(0), IROperand::vreg(lh, 4));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(rh, 4));

		Builder().add_instruction(cmp_factory(lh, rh, output));
		Builder().streg(IROperand::vreg(output, 1), IROperand::const32(0));
		Builder().ret();
	}
	
	void build_and_test(std::function<bool(uint32_t, uint32_t)> cmp_fn) {
		transforms::AllocationWriterTransform awt(allocations_);
		awt.Apply(tc_);

		CompileResult cr (true, stack_frame_, 0xff);

		auto fn = Lower(cr);
		ASSERT_NE(nullptr, fn);

		std::vector<char> regfile_mock(128, 0);
		uint32_t *regfile = (uint32_t*)regfile_mock.data();
		for(int i = 0; i < 1; ++i) {

			uint32_t x = rand(), y = rand();
			regfile[0] = x;
			regfile[1] = y;

			fn(regfile_mock.data(), nullptr);

			ASSERT_EQ(*(uint8_t*)regfile_mock.data(), cmp_fn(x,y));
		}
	}
};

#define MAKE_TEST(name, layout, instruction, comparator) TEST_F(ArchSimBlockJITCmpTest, name) { \
	build_cmp_skeleton_ ## layout([](IRRegId lh, IRRegId rh, IRRegId output){return IRInstruction::instruction(IROperand::vreg(lh, 4), IROperand::vreg(rh, 4), IROperand::vreg(output, 1));}); \
	build_and_test(comparator()); \
}

#define MAKE_TESTS(name_root, instruction, comparator) \
	MAKE_TEST(name_root ## SSS, sss, instruction, comparator) \
	MAKE_TEST(name_root ## SSR, ssr, instruction, comparator) \
	MAKE_TEST(name_root ## SRS, srs, instruction, comparator) \
	MAKE_TEST(name_root ## SRR, srr, instruction, comparator) \
	MAKE_TEST(name_root ## RSS, rss, instruction, comparator) \
	MAKE_TEST(name_root ## RSR, rsr, instruction, comparator) \
	MAKE_TEST(name_root ## RRS, rrs, instruction, comparator) \
	MAKE_TEST(name_root ## RRR, rrr, instruction, comparator)

MAKE_TESTS(cmpeq, cmpeq, std::equal_to<uint32_t>)
MAKE_TESTS(cmpne, cmpne, std::not_equal_to<uint32_t>)
MAKE_TESTS(cmplt, cmplt, std::less<uint32_t>)
MAKE_TESTS(cmplte, cmplte, std::less_equal<uint32_t>)
MAKE_TESTS(cmpgt, cmpgt, std::greater<uint32_t>)
MAKE_TESTS(cmpgte, cmpgte, std::greater_equal<uint32_t>)
MAKE_TESTS(cmpslt, cmpslt, std::less<int32_t>)
MAKE_TESTS(cmpslte, cmpslte, std::less_equal<int32_t>)
MAKE_TESTS(cmpsgt, cmpsgt, std::greater<int32_t>)
MAKE_TESTS(cmpsgte, cmpsgte, std::greater_equal<int32_t>)
