/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * CacheBasedSystemMemoryTranslationModel.cpp
 *
 *  Created on: 8 Sep 2014
 *      Author: harry
 */

#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "gensim/gensim_processor.h"
#include "gensim/gensim_processor_state.h"
#include "translate/TranslationWorkUnit.h"
#include "util/ComponentManager.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <sstream>

using namespace archsim::abi::memory;

CacheBasedSystemMemoryTranslationModel::CacheBasedSystemMemoryTranslationModel()
{

}

CacheBasedSystemMemoryTranslationModel::~CacheBasedSystemMemoryTranslationModel()
{

}

bool CacheBasedSystemMemoryTranslationModel::Initialise(SystemMemoryModel &mem_model)
{
	return true;
}

bool CacheBasedSystemMemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventRead, address, (uint8_t)width);
	}

	llvm::Function *fn = (llvm::Function*)GetOrCreateMemReadFn(insn_ctx.block.region.txln, width, sx);

	llvm::Value *tmp_destination = NULL;
	switch(width) {
		case 1:
			tmp_destination = insn_ctx.block.region.values.mem_read_temp_8;
			break;
		case 2:
			tmp_destination = insn_ctx.block.region.values.mem_read_temp_16;
			break;
		case 4:
			tmp_destination = insn_ctx.block.region.values.mem_read_temp_32;
			break;
		default:
			assert(false);
	}

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;
	fault = builder.CreateCall3(fn, insn_ctx.block.region.values.state_val, address, tmp_destination);

	llvm::Value *value = builder.CreateLoad(tmp_destination);
	value = builder.CreateZExt(value, destinationType);
	builder.CreateStore(value, destination);

	return true;
}

bool CacheBasedSystemMemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventWrite, address, (uint8_t)width);
	}

	llvm::Function *fn = (llvm::Function*)GetOrCreateMemWriteFn(insn_ctx.block.region.txln, width);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;
	fault = builder.CreateCall3(fn, insn_ctx.block.region.values.state_val, address, value);

	return true;
}

static llvm::Type *GetCpuStatePtrType(archsim::translate::llvm::LLVMTranslationContext& llvm_ctx)
{
	return llvm_ctx.types.state_ptr;
}

static llvm::Type *GetReadPointerType(llvm::LLVMContext &ctx, uint32_t width)
{
	switch(width) {
		case 1:
			return llvm::IntegerType::getInt8PtrTy(ctx, 0);
		case 2:
			return llvm::IntegerType::getInt16PtrTy(ctx, 0);
		case 4:
			return llvm::IntegerType::getInt32PtrTy(ctx, 0);
	}
	assert(false);
}


static llvm::Type *GetWriteValueType(llvm::LLVMContext &ctx, uint32_t width)
{
	switch(width) {
		case 1:
			return llvm::IntegerType::getInt8Ty(ctx);
		case 2:
			return llvm::IntegerType::getInt16Ty(ctx);
		case 4:
			return llvm::IntegerType::getInt32Ty(ctx);
	}
	assert(false);
}

static llvm::FunctionType *GetReadFunctionType(archsim::translate::llvm::LLVMTranslationContext& llvm_ctx, int width)
{
	llvm::Type *addr_type = llvm::IntegerType::getInt32Ty(llvm_ctx.llvm_ctx);
	std::vector<llvm::Type *> param_types { GetCpuStatePtrType(llvm_ctx), addr_type, GetReadPointerType(llvm_ctx.llvm_ctx, width) };

	return llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx.llvm_ctx), param_types, false);
}

static llvm::FunctionType *GetWriteFunctionType(archsim::translate::llvm::LLVMTranslationContext& llvm_ctx, int width)
{
	llvm::Type *addr_type = llvm::IntegerType::getInt32Ty(llvm_ctx.llvm_ctx);
	std::vector<llvm::Type *> param_types { GetCpuStatePtrType(llvm_ctx), addr_type, GetWriteValueType(llvm_ctx.llvm_ctx, width) };

	return llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx.llvm_ctx), param_types, false);
}

static llvm::Type *GetCacheEntryType(llvm::LLVMContext &ctx)
{
	// Cache entry struct is {i32, i32, i64} (tag, padding, ptr - tag)
	std::vector<llvm::Type *> struct_members { llvm::Type::getInt32Ty(ctx), llvm::Type::getInt32Ty(ctx), llvm::Type::getInt64Ty(ctx) };

	return llvm::StructType::create(struct_members);
}

static llvm::Type *GetCacheEntryPtrType(llvm::LLVMContext &ctx)
{
	return llvm::PointerType::get(GetCacheEntryType(ctx), 0);
}

static llvm::Type *GetCacheType(llvm::LLVMContext &ctx)
{
	// cache is an array of cache entries of a particular size
	return llvm::ArrayType::get(GetCacheEntryType(ctx), archsim::abi::memory::SMMCache::kCacheSize);
}

llvm::Value *CacheBasedSystemMemoryTranslationModel::GetOrCreateMemReadFn(archsim::translate::llvm::LLVMTranslationContext& llvm_ctx, int width, bool sx)
{
	assert(!sx);

	// First, try and look up the existing function
	std::stringstream fn_name;
	fn_name << "mem_read_fn_" << width;

	llvm::Function *function;
	if((function = (llvm::Function*)llvm_ctx.llvm_module->getFunction(fn_name.str()))) return function;

	// We couldn't find an existing function, so we need to create a new one
	function = (llvm::Function*)llvm_ctx.llvm_module->getOrInsertFunction(fn_name.str(), GetReadFunctionType(llvm_ctx, width));


	auto fn_arg_iterator = function->getArgumentList().begin();;
	llvm::Value *state_val = fn_arg_iterator++;
	llvm::Value *address = fn_arg_iterator++;
	llvm::Value *target_ptr = fn_arg_iterator++;

	// behaviour of the function needs to be:
	// 0. load cache pointer
	// 1. calculate cache index
	// 2. check cache tag
	// 3. if cache tag matches (LIKELY)
	// 3.1 calculate target address
	// 3.2 load from target address
	// 3.3 store loaded value into read pointer
	// 4 if cache tag doesnt match
	// 4.1 call fallback function

	llvm::BasicBlock *bb = llvm::BasicBlock::Create(llvm_ctx.llvm_ctx, "", function);
	llvm::IRBuilder<> builder (bb);

	// Load cache pointer
	llvm::Value *rd_cache = builder.CreateLoad(builder.CreateStructGEP(state_val, gensim::CpuStateEntries::CpuState_smm_read_cache));
	llvm::Type *cacheptrty = llvm::PointerType::get(GetCacheType(llvm_ctx.llvm_ctx), 0);
	rd_cache = builder.CreateBitCast(rd_cache, cacheptrty);

#ifndef FAST_TAG_CALC

	// Calculate cache index
	llvm::Value *cache_idx = builder.CreateLShr(address, archsim::translate::profile::RegionArch::PageBits);
	cache_idx = builder.CreateURem(cache_idx, llvm_ctx.GetConstantInt32(archsim::abi::memory::SMMCache::kCacheSize));

	// Load cache entry
	std::vector<llvm::Value*> cache_gep_params {llvm_ctx.GetConstantInt32(0), cache_idx};
	llvm::Value *cache_entry = builder.CreateGEP(rd_cache, cache_gep_params);

	// Load cache tag
	llvm::Value *cache_tag = builder.CreateStructGEP(cache_entry, 0);
	llvm_ctx.AddAliasAnalysisNode((llvm::Instruction*)cache_tag, translate::llvm::TAG_SMM_CACHE);
	cache_tag = builder.CreateLoad(cache_tag);

#else

	uint32_t mask = SMMCache::kCacheSize - 1;
	mask <<= 12;

	llvm::Value *masked_index = builder.CreateAnd(address, mask);
	masked_index = builder.CreateLShr(masked_index, 8);
	masked_index = builder.CreateZExt(masked_index, llvm_ctx.types.i64);
	llvm::Value *cache_entry = builder.CreateAdd(masked_index, builder.CreatePtrToInt(rd_cache, llvm_ctx.types.i64));
	cache_entry = builder.CreateIntToPtr(cache_entry, GetCacheEntryPtrType(llvm_ctx.llvm_ctx));
	llvm::Value *cache_tag = builder.CreateLoad(builder.CreateStructGEP(cache_entry, 0));

#endif

	// Check cache tag
	llvm::Value *address_tag = builder.CreateAnd(address, ~archsim::translate::profile::RegionArch::PageMask);

	llvm::Value *tag_correct = builder.CreateICmpEQ(cache_tag, address_tag);

	// Now branch, depending on tag_correct
	llvm::BasicBlock *tag_correct_bb = llvm::BasicBlock::Create(llvm_ctx.llvm_ctx, "", function);
	llvm::BasicBlock *tag_incorrect_bb = llvm::BasicBlock::Create(llvm_ctx.llvm_ctx, "", function);

	builder.CreateCondBr(tag_correct, tag_correct_bb, tag_incorrect_bb);

	// Calculate target address
	builder.SetInsertPoint(tag_correct_bb);
	llvm::Value *page_base = builder.CreateLoad(builder.CreateStructGEP(cache_entry, 2));
	llvm::Value *final_addr = builder.CreateAdd(page_base, builder.CreateZExt(address, llvm_ctx.types.i64));

	// Load from target address
	final_addr = builder.CreateIntToPtr(final_addr, GetReadPointerType(llvm_ctx.llvm_ctx, width));
//	llvm_ctx.AddAliasAnalysisNode((llvm::Instruction*)final_addr, translate::llvm::TAG_MEM_ACCESS);
	llvm::Value *read_val = builder.CreateLoad(final_addr);

	// Store into target pointer
	builder.CreateStore(read_val, target_ptr);

	// Return
	builder.CreateRetVoid();

	// Fallback path
	builder.SetInsertPoint(tag_incorrect_bb);

	llvm::Value *fallback_fn;
	switch(width) {
		case 1:
			fallback_fn = llvm_ctx.jit_functions.cpu_read_8;
			break;
		case 2:
			fallback_fn = llvm_ctx.jit_functions.cpu_read_16;
			break;
		case 4:
			fallback_fn = llvm_ctx.jit_functions.cpu_read_32;
			break;
		default:
			assert(false);
	}

	llvm::Value *cpu_ctx = builder.CreateLoad(builder.CreateStructGEP(state_val, 0));
	builder.CreateCall3(fallback_fn, cpu_ctx, address, target_ptr);
	builder.CreateRetVoid();

	return function;
}

llvm::Value *CacheBasedSystemMemoryTranslationModel::GetOrCreateMemWriteFn(archsim::translate::llvm::LLVMTranslationContext& llvm_ctx, int width)
{
	// First, try and look up the existing function
	std::stringstream fn_name;
	fn_name << "mem_write_fn_" << width;

	llvm::Function *function;
	if((function = (llvm::Function*)llvm_ctx.llvm_module->getFunction(fn_name.str()))) return function;

	// We couldn't find an existing function, so we need to create a new one
	function = (llvm::Function*)llvm_ctx.llvm_module->getOrInsertFunction(fn_name.str(), GetWriteFunctionType(llvm_ctx, width));


	auto fn_arg_iterator = function->getArgumentList().begin();;
	llvm::Value *state_val = fn_arg_iterator++;
	llvm::Value *address = fn_arg_iterator++;
	llvm::Value *value = fn_arg_iterator++;

	// behaviour of the function needs to be:
	// 0. load cache pointer
	// 1. calculate cache index
	// 2. check cache tag
	// 3. if cache tag matches (LIKELY)
	// 3.1 calculate target address
	// 3.2 load from target address
	// 3.3 store loaded value into read pointer
	// 4 if cache tag doesnt match
	// 4.1 call fallback function

	llvm::BasicBlock *bb = llvm::BasicBlock::Create(llvm_ctx.llvm_ctx, "", function);
	llvm::IRBuilder<> builder (bb);

	// Load cache pointer
	llvm::Value *rd_cache = builder.CreateLoad(builder.CreateStructGEP(state_val, gensim::CpuStateEntries::CpuState_smm_write_cache));
	llvm::Type *cacheptrty = llvm::PointerType::get(GetCacheType(llvm_ctx.llvm_ctx), 0);
	rd_cache = builder.CreateBitCast(rd_cache, cacheptrty);

#ifndef FAST_TAG_CALC

	// Calculate cache index
	llvm::Value *cache_idx = builder.CreateLShr(address, archsim::translate::profile::RegionArch::PageBits);
	cache_idx = builder.CreateURem(cache_idx, llvm_ctx.GetConstantInt32(archsim::abi::memory::SMMCache::kCacheSize));

	// Load cache entry
	std::vector<llvm::Value*> cache_gep_params {llvm_ctx.GetConstantInt32(0), cache_idx};
	llvm::Value *cache_entry = builder.CreateGEP(rd_cache, cache_gep_params);

	// Load cache tag
	llvm::Value *cache_tag = builder.CreateStructGEP(cache_entry, 0);
	llvm_ctx.AddAliasAnalysisNode((llvm::Instruction*)cache_tag, translate::llvm::TAG_SMM_CACHE);
	cache_tag = builder.CreateLoad(cache_tag);

#else

	uint32_t mask = SMMCache::kCacheSize - 1;
	mask <<= 12;

	llvm::Value *masked_index = builder.CreateAnd(address, mask);
	masked_index = builder.CreateLShr(masked_index, 8);
	masked_index = builder.CreateZExt(masked_index, llvm_ctx.types.i64);
	llvm::Value *cache_entry = builder.CreateAdd(masked_index, builder.CreatePtrToInt(rd_cache, llvm_ctx.types.i64));
	cache_entry = builder.CreateIntToPtr(cache_entry, GetCacheEntryPtrType(llvm_ctx.llvm_ctx));
	llvm::Value *cache_tag = builder.CreateLoad(builder.CreateStructGEP(cache_entry, 0));

#endif

	// Check cache tag
	llvm::Value *address_tag = builder.CreateAnd(address, ~archsim::translate::profile::RegionArch::PageMask);

	llvm::Value *tag_correct = builder.CreateICmpEQ(cache_tag, address_tag);

	// Now branch, depending on tag_correct
	llvm::BasicBlock *tag_correct_bb = llvm::BasicBlock::Create(llvm_ctx.llvm_ctx, "", function);
	llvm::BasicBlock *tag_incorrect_bb = llvm::BasicBlock::Create(llvm_ctx.llvm_ctx, "", function);

	builder.CreateCondBr(tag_correct, tag_correct_bb, tag_incorrect_bb);

	// Calculate target address
	builder.SetInsertPoint(tag_correct_bb);
	llvm::Value *page_base = builder.CreateLoad(builder.CreateStructGEP(cache_entry, 2));
	llvm::Value *final_addr = builder.CreateAdd(page_base, builder.CreateZExt(address, llvm_ctx.types.i64));

	// Load from target address
	final_addr = builder.CreateIntToPtr(final_addr, GetReadPointerType(llvm_ctx.llvm_ctx, width));
//	llvm_ctx.AddAliasAnalysisNode((llvm::Instruction*)final_addr, translate::llvm::TAG_MEM_ACCESS);

	// Store into target pointer
	builder.CreateStore(value, final_addr);

	// Return
	builder.CreateRetVoid();

	// Fallback path
	builder.SetInsertPoint(tag_incorrect_bb);

	llvm::Value *fallback_fn;
	switch(width) {
		case 1:
			fallback_fn = llvm_ctx.jit_functions.cpu_write_8;
			break;
		case 2:
			fallback_fn = llvm_ctx.jit_functions.cpu_write_16;
			break;
		case 4:
			fallback_fn = llvm_ctx.jit_functions.cpu_write_32;
			break;
		default:
			assert(false);
	}

	llvm::Value *cpu_ctx = builder.CreateLoad(builder.CreateStructGEP(state_val, 0));
	builder.CreateCall3(fallback_fn, cpu_ctx, address, value);
	builder.CreateRetVoid();

	return function;
}

::llvm::Value *CacheBasedSystemMemoryTranslationModel::GetCpuInKernelMode(::llvm::IRBuilder<> builder, ::llvm::Value *cpu_state)
{
	llvm::Value *kern = builder.CreateStructGEP(cpu_state, gensim::CpuStateEntries::CpuState_in_kern_mode);
	kern = builder.CreateLoad(kern);
	return kern;
}

void CacheBasedSystemMemoryTranslationModel::Flush()
{

}

void CacheBasedSystemMemoryTranslationModel::Evict(virt_addr_t virt_addr)
{

}
