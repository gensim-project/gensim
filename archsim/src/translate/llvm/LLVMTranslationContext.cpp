/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMTranslationContext.h"

using namespace archsim::translate::tx_llvm;

LLVMTranslationContext::LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::IRBuilder<> &builder, archsim::core::thread::ThreadInstance *thread) : LLVMCtx(ctx), thread_(thread), Builder(builder)
{
	Types.vtype  = llvm::Type::getVoidTy(ctx);
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

	Functions.cpuWrite8 =  (llvm::Function*)Module->getOrInsertFunction("cpuWrite8", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i8);
	Functions.cpuWrite16 = (llvm::Function*)Module->getOrInsertFunction("cpuWrite16", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i16);
	Functions.cpuWrite32 = (llvm::Function*)Module->getOrInsertFunction("cpuWrite32", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i32);
	Functions.cpuWrite64 = (llvm::Function*)Module->getOrInsertFunction("cpuWrite64", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i64);

	Functions.cpuTraceMemWrite8 =  (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite8", Types.vtype, Types.i8Ptr, Types.i64, Types.i8);
	Functions.cpuTraceMemWrite16 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite16", Types.vtype, Types.i8Ptr, Types.i64, Types.i16);
	Functions.cpuTraceMemWrite32 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite32", Types.vtype, Types.i8Ptr, Types.i64, Types.i32);
	Functions.cpuTraceMemWrite64 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite64", Types.vtype, Types.i8Ptr, Types.i64, Types.i64);

	Functions.cpuTraceMemRead8 =  (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead8", Types.vtype, Types.i8Ptr, Types.i64, Types.i8);
	Functions.cpuTraceMemRead16 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead16", Types.vtype, Types.i8Ptr, Types.i64, Types.i16);
	Functions.cpuTraceMemRead32 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead32", Types.vtype, Types.i8Ptr, Types.i64, Types.i32);
	Functions.cpuTraceMemRead64 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead64", Types.vtype, Types.i8Ptr, Types.i64, Types.i64);

	Functions.cpuTraceBankedRegisterWrite = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegBankWrite", Types.vtype, Types.i8Ptr, Types.i32, Types.i32, Types.i64);
	Functions.cpuTraceRegisterWrite = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegWrite", Types.vtype, Types.i8Ptr, Types.i32, Types.i64);
	Functions.cpuTraceBankedRegisterRead = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegBankRead", Types.vtype, Types.i8Ptr, Types.i32, Types.i32, Types.i64);
	Functions.cpuTraceRegisterRead = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegRead", Types.vtype, Types.i8Ptr, Types.i32, Types.i64);

	Functions.TakeException = (llvm::Function*)Module->getOrInsertFunction("cpuTakeException", Types.vtype, Types.i8Ptr, Types.i32, Types.i32);

	Functions.cpuTraceInstruction = (llvm::Function*)Module->getOrInsertFunction("cpuTraceInstruction", Types.vtype, Types.i8Ptr, Types.i64, Types.i32, Types.i32, Types.i32, Types.i32);
	Functions.cpuTraceInsnEnd = (llvm::Function*)Module->getOrInsertFunction("cpuTraceInsnEnd", Types.vtype, Types.i8Ptr);

	Values.reg_file_ptr =    builder.GetInsertBlock()->getParent()->arg_begin();
	Values.state_block_ptr = builder.GetInsertBlock()->getParent()->arg_begin() + 1;
}

llvm::Value* LLVMTranslationContext::GetThreadPtr()
{
	auto ptr = Values.state_block_ptr;
	return Builder.CreateLoad(Builder.CreatePointerCast(ptr, Types.i8Ptr->getPointerTo(0)));
}

llvm::Value* LLVMTranslationContext::AllocateRegister(llvm::Type *type)
{
	if(free_registers_[type].empty()) {
		auto &block = Builder.GetInsertBlock()->getParent()->getEntryBlock();

		if(block.empty()) {
			return new llvm::AllocaInst(type, 0, nullptr, "", &block);
		} else {
			return new llvm::AllocaInst(type, 0, nullptr, "", &*block.begin());
		}
	} else {
		auto reg = free_registers_[type].back();
		free_registers_[type].pop_back();
		return reg;
	}
}

void LLVMTranslationContext::FreeRegister(int width, llvm::Value* v)
{
//	free_registers_[width].push_back(v);
}
