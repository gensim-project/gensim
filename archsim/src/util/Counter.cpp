//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe Counter classes responsible for maintaining and  instantiating
// all kinds of profiling counters in a generic way.
//
// There are two types of Counters, 32-bit counters (i.e. Counter), and
// 64-bit counters (i.e. Counter64).
//
// =====================================================================

#include "util/Counter.h"

using namespace archsim::util;

Counter::Counter() : count_(0)
{
}

Counter64::Counter64() : count_(0)
{
}

#if CONFIG_LLVM
void Counter64::emit_inc(archsim::translate::translate_llvm::LLVMTranslationContext &ctx, llvm::IRBuilder<> &bldr)
{
	assert(false);
#if 0
	uint64_t *const native_ins_ptr = get_ptr();
	llvm::Value *llvm_ins_ptr = llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(twu_ctx.llvm_types.i64Type, (uint64_t)native_ins_ptr, false), twu_ctx.llvm_types.ui64PtrType);
	llvm::Value *loaded_val = bldr.CreateLoad(llvm_ins_ptr);
	llvm::Value *added_val = bldr.CreateAdd(loaded_val, llvm::ConstantInt::get(twu_ctx.llvm_types.i64Type, 1, false));
	bldr.CreateStore(added_val, llvm_ins_ptr);
#endif
}
#endif
