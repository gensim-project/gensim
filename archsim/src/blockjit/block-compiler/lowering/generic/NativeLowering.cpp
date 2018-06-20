/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"

using namespace captive::arch::jit::lowering;
using namespace captive::arch::jit;
using namespace captive::shared;

LoweringResult captive::arch::jit::lowering::NativeLowering(TranslationContext &ctx, wulib::MemAllocator &allocator, const archsim::ArchDescriptor &arch, const archsim::StateBlockDescriptor &state, const CompileResult &compile_result) {
	throw std::logic_error("Cannot lower blockjit for generic simulation target");
	return LoweringResult(nullptr, 0);
}

bool captive::arch::jit::lowering::HasNativeLowering() 
{
	return false;
}