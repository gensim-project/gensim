/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMTranslationContext.h"

using namespace archsim::translate::tx_llvm;

LLVMTranslationContext::LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::IRBuilder<> &builder, archsim::core::thread::ThreadInstance *thread) : LLVMCtx(ctx), thread_(thread), Builder(builder)
{
	Types.i1  = llvm::IntegerType::getInt1Ty(ctx);
	Types.i8  = llvm::IntegerType::getInt8Ty(ctx);
	Types.i8Ptr  = llvm::IntegerType::getInt8PtrTy(ctx, 0);
	Types.i16 = llvm::IntegerType::getInt16Ty(ctx);
	Types.i32 = llvm::IntegerType::getInt32Ty(ctx);
	Types.i64 = llvm::IntegerType::getInt64Ty(ctx);
	Types.i128 = llvm::IntegerType::getInt128Ty(ctx);

	Types.f32 = nullptr;
	Types.f64 = nullptr;

	Module = builder.GetInsertBlock()->getParent()->getParent();

	Functions.ctpop_i32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ctpop, Types.i32);

	Functions.blkRead8 =  (llvm::Function*)Module->getOrInsertFunction("blkRead8", Types.i8, Types.i8Ptr, Types.i64, Types.i32);
	Functions.blkRead16 = (llvm::Function*)Module->getOrInsertFunction("blkRead16", Types.i16, Types.i8Ptr, Types.i64, Types.i32);
	Functions.blkRead32 = (llvm::Function*)Module->getOrInsertFunction("blkRead32", Types.i32, Types.i8Ptr, Types.i64, Types.i32);
	Functions.blkRead64 = (llvm::Function*)Module->getOrInsertFunction("blkRead64", Types.i64, Types.i8Ptr, Types.i64, Types.i32);

	Values.reg_file_ptr =    builder.GetInsertBlock()->getParent()->arg_begin();
	Values.state_block_ptr = builder.GetInsertBlock()->getParent()->arg_begin() + 1;
}

llvm::Value* LLVMTranslationContext::GetThreadPtr(llvm::IRBuilder<>& builder)
{
	llvm::Function *fn;
	UNIMPLEMENTED;
	return nullptr;
}

llvm::Value* LLVMTranslationContext::AllocateRegister(int width)
{
	if(free_registers_[width].empty()) {
		auto type = llvm::IntegerType::getIntNTy(LLVMCtx, width*8);

		auto &block = Builder.GetInsertBlock()->getParent()->getEntryBlock();

		if(block.empty()) {
			return new llvm::AllocaInst(type, 0, nullptr, "", &block);
		} else {
			return new llvm::AllocaInst(type, 0, nullptr, "", &*block.begin());
		}
	} else {
		auto reg = free_registers_[width].back();
		free_registers_[width].pop_back();
		return reg;
	}
}

void LLVMTranslationContext::FreeRegister(int width, llvm::Value* v)
{
	free_registers_[width].push_back(v);
}
