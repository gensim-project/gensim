/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

#include "translate/profile/RegionArch.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"

#include <llvm/IR/BasicBlock.h>

using namespace archsim::translate::adapt;

llvm::Type *GetCacheEntryType(llvm::LLVMContext &ctx) {
	auto i32Ty = llvm::Type::getInt32Ty(ctx);
	auto i8PtrTy = llvm::Type::getInt64Ty(ctx);
	
	// virt addr, phys addr, data pointer
	llvm::StructType *stype = llvm::StructType::get(ctx, {i32Ty, i32Ty, i8PtrTy}, true);
	return stype;
}

bool BlockJITLDMEMCacheLowering::Lower(const captive::shared::IRInstruction*& insn) {	
	const auto &interface = insn->operands[0];
	const auto &offset = insn->operands[1];
	const auto &disp = insn->operands[2];
	const auto &dest = insn->operands[3];
	
	assert(interface.is_constant());
	
	auto cache_entry_type = GetCacheEntryType(GetContext().GetLLVMContext());
	auto final_address = GetBuilder().CreateAdd(GetValueFor(offset), GetValueFor(disp));
	auto cache_ptr = GetContext().GetStateBlockEntryPtr("smm_read_cache", cache_entry_type->getPointerTo(0));
	cache_ptr = GetBuilder().CreateLoad(cache_ptr);
	
	// check alignment
	auto low_bits = GetBuilder().CreateAnd(final_address, dest.size-1);
	auto aligned_access = GetBuilder().CreateICmpEQ(low_bits, llvm::ConstantInt::get(low_bits->getType(), 0));
	
	// TODO: alignment stuff
	
	using archsim::abi::memory::SMMCache;
	using archsim::abi::memory::SMMCacheEntry;
	
	// get cache tag
	auto page_index = GetBuilder().CreateLShr(final_address, archsim::RegionArch::PageBits);
	auto cache_entry_index = GetBuilder().CreateURem(page_index, llvm::ConstantInt::get(page_index->getType(), SMMCache::kCacheSize));
	
	auto cache_entry_ptr = GetBuilder().CreateGEP(cache_ptr, cache_entry_index);
	auto cache_tag = GetBuilder().CreateLoad(GetBuilder().CreateStructGEP(cache_entry_type, cache_entry_ptr, 0));
	auto address_tag = GetBuilder().CreateAnd(final_address, ~archsim::RegionArch::PageMask);
	
	auto tag_matches = GetBuilder().CreateICmpEQ(cache_tag, address_tag);
	
	auto take_hot_path = GetBuilder().CreateAnd(tag_matches, aligned_access);
			
	auto hit_block = llvm::BasicBlock::Create(GetContext().GetLLVMContext(), "", GetBuilder().GetInsertBlock()->getParent());
	auto miss_block = llvm::BasicBlock::Create(GetContext().GetLLVMContext(), "", GetBuilder().GetInsertBlock()->getParent());
	auto final_block = llvm::BasicBlock::Create(GetContext().GetLLVMContext(), "", GetBuilder().GetInsertBlock()->getParent());
	GetBuilder().CreateCondBr(take_hot_path, hit_block, miss_block);
	
	llvm::Value *hit_value, *miss_value;
	// hit block
	{
		GetBuilder().SetInsertPoint(hit_block);
		
		// calculate final address to access
		auto addend = GetBuilder().CreateLoad(GetBuilder().CreateStructGEP(cache_entry_type, cache_entry_ptr, 2));
		auto extended_final_address = GetBuilder().CreateZExt(final_address, addend->getType());
		auto final_host_address = GetBuilder().CreateAdd(extended_final_address, addend);
		
		auto address_ptr = GetBuilder().CreateIntToPtr(final_host_address, GetContext().GetLLVMType(dest)->getPointerTo(0));
		hit_value = GetBuilder().CreateLoad(address_ptr);
		
		GetBuilder().CreateBr(final_block);
	}
	
	// miss block
	{
		GetBuilder().SetInsertPoint(miss_block);
		
		llvm::Value *target_fn = nullptr;
		switch(dest.size) {
			case 1:
				target_fn = GetContext().GetValues().blkRead8Ptr; break;
			case 2:
				target_fn = GetContext().GetValues().blkRead16Ptr; break;
			case 4:
				target_fn = GetContext().GetValues().blkRead32Ptr; break;
			case 8:
			default:
				UNIMPLEMENTED;
		}
		
		auto interface_value = GetValueFor(interface);
		
		miss_value = GetBuilder().CreateCall(target_fn, { GetContext().GetThreadPtrPtr(), final_address, interface_value });
		
		GetBuilder().CreateBr(final_block);
	}
	
	
	// final block
	GetBuilder().SetInsertPoint(final_block);

	llvm::PHINode *phi = GetBuilder().CreatePHI(hit_value->getType(), 2);
	phi->addIncoming(hit_value, hit_block);
	phi->addIncoming(miss_value, miss_block);
	SetValueFor(dest, phi);
	
	insn++;
	
	return true;
}

bool BlockJITSTMEMCacheLowering::Lower(const captive::shared::IRInstruction*& insn) {
	const auto &interface = insn->operands[0];
	const auto &value = insn->operands[1];
	const auto &disp = insn->operands[2];
	const auto &offset = insn->operands[3];
	
	auto cache_entry_type = GetCacheEntryType(GetContext().GetLLVMContext());
	auto final_address = GetBuilder().CreateAdd(GetValueFor(offset), GetValueFor(disp));
	auto cache_ptr = GetContext().GetStateBlockEntryPtr("smm_write_cache", cache_entry_type->getPointerTo(0));
	cache_ptr = GetBuilder().CreateLoad(cache_ptr);
	
	auto data_value = GetValueFor(value);
	
	// check alignment
	auto low_bits = GetBuilder().CreateAnd(final_address, value.size-1);
	auto aligned_access = GetBuilder().CreateICmpEQ(low_bits, llvm::ConstantInt::get(low_bits->getType(), 0));
	
	// TODO: alignment stuff
	
	using archsim::abi::memory::SMMCache;
	using archsim::abi::memory::SMMCacheEntry;
	
	// get cache tag
	auto page_index = GetBuilder().CreateLShr(final_address, archsim::RegionArch::PageBits);
	auto cache_entry_index = GetBuilder().CreateURem(page_index, llvm::ConstantInt::get(page_index->getType(), SMMCache::kCacheSize));
	
	auto cache_entry_ptr = GetBuilder().CreateGEP(cache_ptr, cache_entry_index);
	auto cache_tag = GetBuilder().CreateLoad(GetBuilder().CreateStructGEP(cache_entry_type, cache_entry_ptr, 0));
	auto address_tag = GetBuilder().CreateAnd(final_address, ~archsim::RegionArch::PageMask);
	
	auto tag_matches = GetBuilder().CreateICmpEQ(cache_tag, address_tag);
	
	auto take_hot_path = GetBuilder().CreateAnd(tag_matches, aligned_access);
			
	auto hit_block = llvm::BasicBlock::Create(GetContext().GetLLVMContext(), "", GetBuilder().GetInsertBlock()->getParent());
	auto miss_block = llvm::BasicBlock::Create(GetContext().GetLLVMContext(), "", GetBuilder().GetInsertBlock()->getParent());
	auto final_block = llvm::BasicBlock::Create(GetContext().GetLLVMContext(), "", GetBuilder().GetInsertBlock()->getParent());
	GetBuilder().CreateCondBr(take_hot_path, hit_block, miss_block);
	
	// hit block
	{
		GetBuilder().SetInsertPoint(hit_block);
		
		// calculate final address to access
		auto addend = GetBuilder().CreateLoad(GetBuilder().CreateStructGEP(cache_entry_type, cache_entry_ptr, 2));
		auto extended_final_address = GetBuilder().CreateZExt(final_address, addend->getType());
		auto final_host_address = GetBuilder().CreateAdd(extended_final_address, addend);
		
		auto address_ptr = GetBuilder().CreateIntToPtr(final_host_address, GetContext().GetLLVMType(value)->getPointerTo(0));
		GetBuilder().CreateStore(data_value, address_ptr);
		
		GetBuilder().CreateBr(final_block);
	}
	
	// miss block
	{
		GetBuilder().SetInsertPoint(miss_block);
		
		llvm::Value *target_fn = nullptr;
		switch(value.size) {
			case 1:
				target_fn = GetContext().GetValues().blkWrite8Ptr; break;
			case 2:
				target_fn = GetContext().GetValues().blkWrite16Ptr; break;
			case 4:
				target_fn = GetContext().GetValues().blkWrite32Ptr; break;
			case 8:
			default:
				UNIMPLEMENTED;
		}
		
		auto interface_value = GetValueFor(interface);
		
		GetBuilder().CreateCall(target_fn, { GetContext().GetThreadPtr(), interface_value, final_address, data_value });
		
		GetBuilder().CreateBr(final_block);
	}
	
	
	// final block
	GetBuilder().SetInsertPoint(final_block);

	insn++;
	
	return true;
}
