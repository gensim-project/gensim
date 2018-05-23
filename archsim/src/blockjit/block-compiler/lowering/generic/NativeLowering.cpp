/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"

using namespace captive::arch::jit::lowering;
using namespace captive::arch::jit;
using namespace captive::shared;

LoweringResult captive::arch::jit::lowering::NativeLowering(TranslationContext &ctx, wulib::MemAllocator &allocator, archsim::core::thread::ThreadInstance *thread, const CompileResult &compile_result) {
	throw std::logic_error("Cannot lower blockjit for generic simulation target");
	return LoweringResult(nullptr, 0);
}
