/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_translate.h"
#include "translate/llvm/LLVMTranslationContext.h"

#include <llvm/IR/Constants.h>

using namespace gensim;

llvm::Value *BaseLLVMTranslate::EmitRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void* irbuilder, int size, int offset)
{
	llvm::IRBuilder<> &builder = *(llvm::IRBuilder<>*)irbuilder;
	llvm::Value *ptr = GetRegisterPtr(ctx, irbuilder, size, offset);
	return builder.CreateLoad(ptr);
}

bool BaseLLVMTranslate::EmitRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void* irbuilder, int size, int offset, llvm::Value *value)
{
	llvm::IRBuilder<> &builder = *(llvm::IRBuilder<>*)irbuilder;
	llvm::Value *ptr = GetRegisterPtr(ctx, irbuilder, size, offset);
	builder.CreateStore(value, ptr);
	return true;
}

llvm::Value* BaseLLVMTranslate::EmitMemoryRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void* irbuilder, int size, llvm::Value* address)
{
	llvm::Function *fn_ptr = nullptr;
	switch(size) {
		case 8:
			fn_ptr = ctx.Functions.blkRead8;
			break;
		case 16:
			fn_ptr = ctx.Functions.blkRead16;
			break;
		case 32:
			fn_ptr = ctx.Functions.blkRead32;
			break;
		case 64:
			fn_ptr = ctx.Functions.blkRead64;
			break;
		default:
			UNIMPLEMENTED;
	}

	return ctx.Builder.CreateCall(fn_ptr, {ctx.Values.state_block_ptr, address, llvm::ConstantInt::get(ctx.Types.i32, 0)});
}

void BaseLLVMTranslate::EmitMemoryWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void* irbuilder, int size, llvm::Value* address, llvm::Value* value)
{

}

llvm::Value* BaseLLVMTranslate::GetRegisterPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void* irbuilder, int size, int offset)
{
	llvm::IRBuilder<> &builder = *(llvm::IRBuilder<>*)irbuilder;
	llvm::Value *ptr = GetRegfilePtr(ctx);
	ptr = builder.CreatePtrToInt(ptr, ctx.Types.i64);
	ptr = builder.CreateAdd(ptr, llvm::ConstantInt::get(ctx.Types.i64, offset));
	ptr = builder.CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size*8));
	return ptr;
}

llvm::Value* BaseLLVMTranslate::GetRegfilePtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx)
{
	return ctx.Values.reg_file_ptr;
}
