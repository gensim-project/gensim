/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"

#include "blockjit/block-compiler/lowering/x86/X86Encoder.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit);

using namespace captive::arch::jit::lowering;
using namespace captive::arch::jit;
using namespace captive::shared;

LoweringResult captive::arch::jit::lowering::NativeLowering(TranslationContext &ctx, wulib::MemAllocator &allocator, const archsim::ArchDescriptor &arch, const archsim::StateBlockDescriptor &state, const CompileResult &compile_result) {
	lowering::x86::X86Encoder encoder(allocator);
	lowering::x86::X86LoweringContext lowering(compile_result.StackFrameSize, encoder, arch, state, compile_result.UsedPhysRegs);
	lowering.Prepare(ctx);
	
	if(!lowering.Lower(ctx)) {
		LC_ERROR(LogBlockJit) << "Failed to lower block";
		return LoweringResult(0,0);
	}

	block_txln_fn fn = (block_txln_fn)encoder.get_buffer();
	fn = (block_txln_fn)allocator.Reallocate(encoder.get_buffer(), encoder.get_buffer_size());
	return LoweringResult(fn, encoder.get_buffer_size());
}
