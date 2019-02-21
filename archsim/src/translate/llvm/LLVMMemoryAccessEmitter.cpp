/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMMemoryAccessEmitter.h"
#include "translate/llvm/LLVMTranslationContext.h"

using namespace archsim::translate::translate_llvm;

LLVMMemoryAccessEmitter::LLVMMemoryAccessEmitter(LLVMTranslationContext& ctx) : ctx_(ctx)
{

}

LLVMMemoryAccessEmitter::~LLVMMemoryAccessEmitter()
{

}

void LLVMMemoryAccessEmitter::Reset()
{

}

void LLVMMemoryAccessEmitter::Finalize()
{

}

BaseLLVMMemoryAccessEmitter::BaseLLVMMemoryAccessEmitter(LLVMTranslationContext& ctx) : LLVMMemoryAccessEmitter(ctx)
{

}

llvm::Value* BaseLLVMMemoryAccessEmitter::EmitMemoryRead(llvm::IRBuilder<>& builder, llvm::Value* address, int size_in_bytes, int interface, int mode)
{
	llvm::Function *fn_ptr = nullptr;
	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			fn_ptr = GetCtx().Functions.blkRead8;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead8;
			break;
		case 2:
			fn_ptr = GetCtx().Functions.blkRead16;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead16;
			break;
		case 4:
			fn_ptr = GetCtx().Functions.blkRead32;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead32;
			break;
		case 8:
			fn_ptr = GetCtx().Functions.blkRead64;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead64;
			break;
		default:
			UNIMPLEMENTED;
	}

	llvm::Value *value = builder.CreateCall(fn_ptr, {GetCtx().Values.state_block_ptr, address, llvm::ConstantInt::get(GetCtx().Types.i32, interface)});

	if(archsim::options::Trace) {
		if(trace_fn_ptr != nullptr) {
			builder.CreateCall(trace_fn_ptr, {GetCtx().GetThreadPtr(builder), address, value});
		}
	}

	return value;
}

void BaseLLVMMemoryAccessEmitter::EmitMemoryWrite(llvm::IRBuilder<>& builder, llvm::Value* address, llvm::Value* value, int size_in_bytes, int interface, int mode)
{
	llvm::Function *fn_ptr = nullptr;
	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			fn_ptr = GetCtx().Functions.cpuWrite8;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite8;
			break;
		case 2:
			fn_ptr = GetCtx().Functions.cpuWrite16;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite16;
			break;
		case 4:
			fn_ptr = GetCtx().Functions.cpuWrite32;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite32;
			break;
		case 8:
			fn_ptr = GetCtx().Functions.cpuWrite64;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite64;
			break;
		default:
			UNIMPLEMENTED;
	}

	builder.CreateCall(fn_ptr, {GetCtx().GetThreadPtr(builder), llvm::ConstantInt::get(GetCtx().Types.i32, interface), address, value});

	if(archsim::options::Trace) {
		if(trace_fn_ptr != nullptr) {
			builder.CreateCall(trace_fn_ptr, {GetCtx().GetThreadPtr(builder), address, value});
		}
	}
}

CacheLLVMMemoryAccessEmitter::CacheLLVMMemoryAccessEmitter(LLVMTranslationContext& ctx) : LLVMMemoryAccessEmitter(ctx)
{

}

llvm::StructType* CacheLLVMMemoryAccessEmitter::GetCacheEntryType()
{
	return llvm::StructType::get(GetCtx().LLVMCtx, {GetCtx().Types.i64, GetCtx().Types.i64, GetCtx().Types.i8Ptr, GetCtx().Types.i64});
}

llvm::Type* CacheLLVMMemoryAccessEmitter::GetCacheType()
{
	return GetCacheEntryType()->getPointerTo(0);
}

llvm::Value* CacheLLVMMemoryAccessEmitter::GetCache(llvm::IRBuilder<>& builder, const std::string& cache_name)
{
	llvm::Value *cache_ptr = builder.CreatePointerCast(GetCtx().GetStateBlockPointer(builder, cache_name), GetCacheType()->getPointerTo(0));
	llvm::LoadInst *cache = builder.CreateLoad(cache_ptr);

	return cache;
}

llvm::Value* CacheLLVMMemoryAccessEmitter::GetCacheEntry(llvm::IRBuilder<>& builder, llvm::Value* cache, llvm::Value* address)
{
	llvm::Value *page_index = builder.CreateLShr(address, 12);
	llvm::Value *cache_entry_index = builder.CreateURem(page_index, llvm::ConstantInt::get(GetCtx().Types.i64, 1024));

	// try really hard to get good codegen for cache entry lookup
	llvm::Value *cache_entry_offset = builder.CreateMul(cache_entry_index, llvm::ConstantInt::get(GetCtx().Types.i64, 32));
	llvm::Value *cache_naked_ptr = builder.CreatePtrToInt(cache, GetCtx().Types.i64);

	llvm::Value *cache_entry_ptr = builder.CreateAdd(cache_naked_ptr, cache_entry_offset);
	cache_entry_ptr = builder.CreateIntToPtr(cache_entry_ptr, GetCacheEntryType()->getPointerTo(0));

	return cache_entry_ptr;
}

llvm::Value* CacheLLVMMemoryAccessEmitter::GetCacheEntry_Data(llvm::IRBuilder<>& builder, llvm::Value* entry)
{
	return builder.CreateLoad(builder.CreateStructGEP(GetCacheEntryType(), entry, 2));
}

llvm::Value* CacheLLVMMemoryAccessEmitter::GetCacheEntry_VirtualTag(llvm::IRBuilder<>& builder, llvm::Value* entry)
{
	return builder.CreateLoad(builder.CreateStructGEP(GetCacheEntryType(), entry, 0));
}


llvm::Value* CacheLLVMMemoryAccessEmitter::EmitMemoryRead(llvm::IRBuilder<>& builder, llvm::Value* address, int size_in_bytes, int interface, int mode)
{
	auto cache_ptr = GetCache(builder, "mem_cache_0_3_read");

	auto cache_entry = GetCacheEntry(builder, cache_ptr, address);

	llvm::Value *tag = GetCacheEntry_VirtualTag(builder, cache_entry);

	// does tag match? make sure end of memory access falls on correct page
	llvm::Value *offset_page_base = builder.CreateAdd(address, llvm::ConstantInt::get(GetCtx().Types.i64, size_in_bytes-1));
	offset_page_base = builder.CreateAnd(offset_page_base, ~archsim::Address::PageMask);

	llvm::Value *tag_match = builder.CreateICmpEQ(tag, offset_page_base);
	llvm::BasicBlock *start_block = builder.GetInsertBlock();
	llvm::BasicBlock *nomatch_block = llvm::BasicBlock::Create(GetCtx().LLVMCtx, "", builder.GetInsertBlock()->getParent());
	llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(GetCtx().LLVMCtx, "", builder.GetInsertBlock()->getParent());

	llvm::Value *page_base = GetCacheEntry_Data(builder, cache_entry);
	llvm::Value *page_offset = builder.CreateAnd(address, archsim::Address::PageMask);
	llvm::Value *ptr = builder.CreateGEP(page_base, page_offset);
	ptr = builder.CreatePointerCast(ptr, llvm::IntegerType::getIntNPtrTy(GetCtx().LLVMCtx, size_in_bytes*8));
	llvm::Value *match_value = builder.CreateLoad(ptr);

	auto br = builder.CreateCondBr(tag_match, continue_block, nomatch_block);

	builder.SetInsertPoint(nomatch_block);

	llvm::Function *fn_ptr = nullptr;
	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			fn_ptr = GetCtx().Functions.blkRead8;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead8;
			break;
		case 2:
			fn_ptr = GetCtx().Functions.blkRead16;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead16;
			break;
		case 4:
			fn_ptr = GetCtx().Functions.blkRead32;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead32;
			break;
		case 8:
			fn_ptr = GetCtx().Functions.blkRead64;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemRead64;
			break;
		default:
			UNIMPLEMENTED;
	}


	llvm::Value *nomatch_value = builder.CreateCall(fn_ptr, {GetCtx().Values.state_block_ptr, address, llvm::ConstantInt::get(GetCtx().Types.i32, interface)});

	builder.CreateBr(continue_block);

	builder.SetInsertPoint(continue_block);
	auto value = builder.CreatePHI(llvm::IntegerType::getIntNTy(GetCtx().LLVMCtx, size_in_bytes*8), 2);
	value->addIncoming(match_value, start_block);
	value->addIncoming(nomatch_value, nomatch_block);

	if(archsim::options::Trace) {
		if(trace_fn_ptr != nullptr) {
			builder.CreateCall(trace_fn_ptr, {GetCtx().GetThreadPtr(builder), address, value});
		}
	}

	return value;
}

void CacheLLVMMemoryAccessEmitter::EmitMemoryWrite(llvm::IRBuilder<>& builder, llvm::Value* address, llvm::Value* value, int size_in_bytes, int interface, int mode)
{
	auto cache_ptr = GetCache(builder, "mem_cache_0_3_write");

	auto cache_entry = GetCacheEntry(builder, cache_ptr, address);

	llvm::Value *tag = GetCacheEntry_VirtualTag(builder, cache_entry);

	// does tag match?
	llvm::Value *offset_page_base = builder.CreateAdd(address, llvm::ConstantInt::get(GetCtx().Types.i64, size_in_bytes-1));
	offset_page_base = builder.CreateAnd(offset_page_base, ~archsim::Address::PageMask);

	llvm::Value *tag_match = builder.CreateICmpEQ(tag, offset_page_base);
	llvm::BasicBlock *match_block = llvm::BasicBlock::Create(GetCtx().LLVMCtx, "", builder.GetInsertBlock()->getParent());
	llvm::BasicBlock *nomatch_block = llvm::BasicBlock::Create(GetCtx().LLVMCtx, "", builder.GetInsertBlock()->getParent());
	llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(GetCtx().LLVMCtx, "", builder.GetInsertBlock()->getParent());

	auto br = builder.CreateCondBr(tag_match, match_block, nomatch_block);

	builder.SetInsertPoint(match_block);

	llvm::Value *page_base = GetCacheEntry_Data(builder, cache_entry);
	llvm::Value *page_offset = builder.CreateAnd(address, archsim::Address::PageMask);
	llvm::Value *ptr = builder.CreateGEP(page_base, page_offset);
	ptr = builder.CreatePointerCast(ptr, llvm::IntegerType::getIntNPtrTy(GetCtx().LLVMCtx, size_in_bytes*8));
	builder.CreateStore(value, ptr);

	builder.CreateBr(continue_block);

	builder.SetInsertPoint(nomatch_block);

	llvm::Function *fn_ptr = nullptr;
	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			fn_ptr = GetCtx().Functions.cpuWrite8;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite8;
			break;
		case 2:
			fn_ptr = GetCtx().Functions.cpuWrite16;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite16;
			break;
		case 4:
			fn_ptr = GetCtx().Functions.cpuWrite32;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite32;
			break;
		case 8:
			fn_ptr = GetCtx().Functions.cpuWrite64;
			trace_fn_ptr = GetCtx().Functions.cpuTraceMemWrite64;
			break;
		default:
			UNIMPLEMENTED;
	}

	builder.CreateCall(fn_ptr, {GetCtx().GetThreadPtr(builder), llvm::ConstantInt::get(GetCtx().Types.i32, interface), address, value});

	builder.CreateBr(continue_block);

	builder.SetInsertPoint(continue_block);

	if(archsim::options::Trace) {
		if(trace_fn_ptr != nullptr) {
			builder.CreateCall(trace_fn_ptr, {GetCtx().GetThreadPtr(builder), address, value});
		}
	}
}

void CacheLLVMMemoryAccessEmitter::Reset()
{
}
