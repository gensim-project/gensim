/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>

#include "translate/llvm/LLVMRegisterOptimisationPass.h"
#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/IRBuilder.h>

using namespace archsim::translate::translate_llvm;

TEST(Archsim_LLVM_Transform_DSE, Test1)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 1)});
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateCondBr(llvm::ConstantInt::get(i1ty, 0), b1_block, b2_block);

	builder.SetInsertPoint(b2_block);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 0);
}

TEST(Archsim_LLVM_Transform_DSE, Test2)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 1)});
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);
	auto dead_store = builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateCondBr(llvm::ConstantInt::get(i1ty, 0), b1_block, b2_block);

	builder.SetInsertPoint(b2_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 1);
	ASSERT_EQ(dead_stores.at(0), dead_store);
}


TEST(Archsim_LLVM_Transform_DSE, Test3)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);
	llvm::BasicBlock *b3_block = llvm::BasicBlock::Create(ctx, "b3", fn);
	llvm::BasicBlock *b4_block = llvm::BasicBlock::Create(ctx, "b4", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 1)});
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);
	builder.CreateCondBr(llvm::ConstantInt::get(i1ty, 0), b2_block, b3_block);

	builder.SetInsertPoint(b2_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b3_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr1);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b4_block);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 0);
}

TEST(Archsim_LLVM_Transform_DSE, Test4)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);
	llvm::BasicBlock *b3_block = llvm::BasicBlock::Create(ctx, "b3", fn);
	llvm::BasicBlock *b4_block = llvm::BasicBlock::Create(ctx, "b4", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 1)});
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);
	builder.CreateCondBr(llvm::ConstantInt::get(i1ty, 0), b2_block, b3_block);

	builder.SetInsertPoint(b2_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b3_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b4_block);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 0);
}

TEST(Archsim_LLVM_Transform_DSE, Test5)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);
	llvm::BasicBlock *b3_block = llvm::BasicBlock::Create(ctx, "b3", fn);
	llvm::BasicBlock *b4_block = llvm::BasicBlock::Create(ctx, "b4", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 1)});
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);
	builder.CreateCondBr(llvm::ConstantInt::get(i1ty, 0), b2_block, b3_block);

	builder.SetInsertPoint(b2_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr1);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b3_block);
	auto dead_store = builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b4_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 1);
	ASSERT_EQ(dead_stores.at(0), dead_store);
}

TEST(Archsim_LLVM_Transform_DSE, Test_Unknown_Bounds)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);
	llvm::BasicBlock *b3_block = llvm::BasicBlock::Create(ctx, "b3", fn);
	llvm::BasicBlock *b4_block = llvm::BasicBlock::Create(ctx, "b4", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 1)});
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);
	builder.CreateCondBr(llvm::ConstantInt::get(i1ty, 0), b2_block, b3_block);

	builder.SetInsertPoint(b2_block);

	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr1);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b3_block);
	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);
	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b4_block);

	auto value = (llvm::Value*)builder.CreateLoad(ptr0);
	value = builder.CreateInBoundsGEP(arg0, {value});
	value = builder.CreateLoad(value);

	builder.CreateStore(llvm::ConstantInt::get(i8ty, 0), ptr0);

	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 0);
}

TEST(Archsim_LLVM_Transform_DSE, Test_Overlaps)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i32ptrty = llvm::IntegerType::getInt32PtrTy(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);
	llvm::BasicBlock *b3_block = llvm::BasicBlock::Create(ctx, "b3", fn);
	llvm::BasicBlock *b4_block = llvm::BasicBlock::Create(ctx, "b4", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::ConstantInt *c1_i8 = llvm::ConstantInt::get(i8ty, 1);
	llvm::ConstantInt *c1_i32 = llvm::ConstantInt::get(i32ty, 1);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	ptr1 = builder.CreatePointerCast(ptr1, i32ptrty);
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);

	auto dead_store = builder.CreateStore(c1_i8, ptr0);
	builder.CreateStore(c1_i32, ptr1);

	builder.CreateBr(b4_block);

	builder.SetInsertPoint(b4_block);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 1);
	ASSERT_EQ(dead_stores.at(0), dead_store);
}

TEST(Archsim_LLVM_Transform_DSE, Test_Overlaps2)
{
	llvm::LLVMContext ctx;

	auto mod = new llvm::Module("mod", ctx);

	auto i1ty = llvm::IntegerType::getInt1Ty(ctx);
	auto i32ty = llvm::IntegerType::getInt32Ty(ctx);
	auto i32ptrty = llvm::IntegerType::getInt32PtrTy(ctx);
	auto i8ty = llvm::IntegerType::getInt8Ty(ctx);
	auto i8ptrty = llvm::IntegerType::getInt8PtrTy(ctx);

	auto fn_ty = llvm::FunctionType::get(i32ty, {i8ptrty, i8ptrty}, false);

	auto fn = (llvm::Function*)mod->getOrInsertFunction("test_fn", fn_ty);
	auto arg0 = &*fn->arg_begin();

	ASSERT_NE(nullptr, fn);

	llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

	llvm::BasicBlock *b1_block = llvm::BasicBlock::Create(ctx, "b1", fn);
	llvm::BasicBlock *b2_block = llvm::BasicBlock::Create(ctx, "b2", fn);

	llvm::IRBuilder<> builder(entry_block);

	llvm::ConstantInt *c1_i8 = llvm::ConstantInt::get(i8ty, 1);
	llvm::ConstantInt *c1_i32 = llvm::ConstantInt::get(i32ty, 1);

	llvm::Value *ptr0 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	llvm::Value *ptr1 = builder.CreateInBoundsGEP(arg0, {llvm::ConstantInt::get(i32ty, 0)});
	ptr1 = builder.CreatePointerCast(ptr1, i32ptrty);
	builder.CreateBr(b1_block);

	builder.SetInsertPoint(b1_block);

	builder.CreateStore(c1_i32, ptr1);
	builder.CreateStore(c1_i8, ptr0);

	builder.CreateBr(b2_block);

	builder.SetInsertPoint(b2_block);
	builder.CreateRet(llvm::ConstantInt::get(i32ty, 0));

	archsim::translate::translate_llvm::LLVMRegisterOptimisationPass rop;
	auto dead_stores = rop.getDeadStores(*fn);

	ASSERT_EQ(dead_stores.size(), 0);
}
