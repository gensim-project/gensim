/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * abi/memory/SystemMemoryTranslationModel.cpp
 */

#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "gensim/gensim_processor_state.h"
#include "util/ComponentManager.h"

using namespace archsim::abi::memory;

BaseSystemMemoryTranslationModel::BaseSystemMemoryTranslationModel()
{

}

BaseSystemMemoryTranslationModel::~BaseSystemMemoryTranslationModel()
{

}

bool BaseSystemMemoryTranslationModel::Initialise(SystemMemoryModel &mem_model)
{
	return true;
}

bool BaseSystemMemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	return MemoryTranslationModel::EmitMemoryRead(insn_ctx, width, sx, fault, address, destinationType, destination);
}

bool BaseSystemMemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	return MemoryTranslationModel::EmitMemoryWrite(insn_ctx, width, fault, address, value);
}

bool BaseSystemMemoryTranslationModel::EmitPerformTranslation(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *virt_address, llvm::Value *&phys_address, llvm::Value *&fault)
{
	llvm::Value *phys_addr_slot = insn_ctx.GetSlot("base_txln_phys_ptr", insn_ctx.txln.types.i32);

	fault = insn_ctx.builder.CreateCall3(insn_ctx.txln.jit_functions.cpu_translate, insn_ctx.values.cpu_ctx_val, virt_address, phys_addr_slot);
	phys_address = insn_ctx.builder.CreateLoad(phys_addr_slot);

	return true;
}

void BaseSystemMemoryTranslationModel::Flush()
{

}

void BaseSystemMemoryTranslationModel::Evict(virt_addr_t virt_addr)
{

}
