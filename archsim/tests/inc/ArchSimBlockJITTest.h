/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ArchSimBlockJITTest.h
 * Author: harry
 *
 * Created on 14 June 2018, 10:47
 */

#ifndef ARCHSIMBLOCKJITTEST_H
#define ARCHSIMBLOCKJITTEST_H

#include "core/arch/ArchDescriptor.h"
#include "core/thread/StateBlock.h"
#include "util/MemAllocator.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/transforms/Transform.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"
#include "blockjit/IRBuilder.h"
#include "blockjit/IROperand.h"
#include "blockjit/IRInstruction.h"

#include <gtest/gtest.h>

static archsim::ArchDescriptor GetArch() {
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
	
	static const int kIterations = 0x100000;
	
	archsim::ArchDescriptor Arch;
	archsim::StateBlockDescriptor StateBlockDescriptor;
	wulib::SimpleZoneMemAllocator Allocator;
	
	captive::shared::block_txln_fn CompileAndLower() {
		captive::arch::jit::BlockCompiler block_compiler (tc_, 0, Allocator);
	
		auto result = block_compiler.compile(false);

		return Lower(result);
	}
	
	captive::shared::block_txln_fn Lower(const captive::arch::jit::CompileResult &cr) {
		return captive::arch::jit::lowering::NativeLowering(tc_, Allocator, Arch, StateBlockDescriptor, cr).Function;
	}
	
	captive::shared::IRRegId StackValue(uint64_t value, uint8_t size) {
		captive::shared::IRRegId reg = tc_.alloc_reg(size);
		allocations_[reg] = std::make_pair(captive::shared::IROperand::ALLOCATED_STACK, stack_frame_);
		stack_frame_ += 8;
		
		Builder().mov(captive::shared::IROperand::constant(value, size), captive::shared::IROperand::vreg(reg, size));
		return reg;
	}
	
	captive::shared::IRRegId RegValue(uint64_t value, uint8_t size, uint8_t reg_id) {
		captive::shared::IRRegId reg = tc_.alloc_reg(size);
		allocations_[reg] = std::make_pair(captive::shared::IROperand::ALLOCATED_REG, reg_id);
		
		builder_.mov(captive::shared::IROperand::constant(value, size), captive::shared::IROperand::vreg(reg, size));
		return reg;
	}
	
	captive::shared::IRRegId AllocateReg(uint32_t size) {
		captive::shared::IRRegId reg = tc_.alloc_reg(size);
		allocations_[reg] = std::make_pair(captive::shared::IROperand::ALLOCATED_REG, next_reg_++);
		return reg;
	}
	
	captive::shared::IRRegId AllocateStack(uint32_t size) {
		captive::shared::IRRegId reg = tc_.alloc_reg(size);
		allocations_[reg] = std::make_pair(captive::shared::IROperand::ALLOCATED_STACK, stack_frame_);
		stack_frame_ += 8;
		return reg;
	}
	
	class StackTag {};
	class RegTag {};
	class ConstantTag {};
	
	static const uint32_t kUConstant = 0x1000;
	template<uint32_t modulus=0xffffffff> class RandValueGen { public: operator uint32_t() { return ((uint32_t)rand()) % modulus; } };
	class ConstValueGen { public: operator uint32_t() { return kUConstant; } };
	
	template<typename t> captive::shared::IRRegId Allocate(t, uint32_t size);
	
	void SetUp() override {
		tc_.clear();
		stack_frame_ = 0;
		allocations_.clear();
		builder_.SetContext(&tc_);
		builder_.SetBlock(tc_.alloc_block());
		next_reg_ = 0;
	}

	captive::shared::IRBuilder &Builder() { return builder_; }
	
	captive::arch::jit::transforms::AllocationWriterTransform::allocations_t allocations_;
	uint32_t stack_frame_;
	captive::arch::jit::TranslationContext tc_;
	captive::shared::IRBuilder builder_;
	int next_reg_;
};

template<> inline captive::shared::IRRegId ArchSimBlockJITTest::Allocate<ArchSimBlockJITTest::StackTag>(ArchSimBlockJITTest::StackTag, uint32_t size) { return AllocateStack(size); }
template<> inline captive::shared::IRRegId ArchSimBlockJITTest::Allocate<ArchSimBlockJITTest::RegTag>(ArchSimBlockJITTest::RegTag, uint32_t size) { return AllocateReg(size); }

#endif /* ARCHSIMBLOCKJITTEST_H */

