/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/memory/MemoryEventHandlerTranslator.h"

#include "translate/llvm/LLVMTranslationContext.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace archsim::abi::memory;

extern "C" void DefaultMemoryEventHandlerTranslatorRaiseEvent(gensim::Processor& cpu, archsim::abi::memory::MemoryEventHandler& handler, archsim::abi::memory::MemoryModel::MemoryEventType type, uint32_t addr, uint32_t width)
{
	handler.HandleEvent(cpu, type, addr, width);
}

bool DefaultMemoryEventHandlerTranslator::EmitEventHandler(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, archsim::abi::memory::MemoryEventHandler& handler, archsim::abi::memory::MemoryModel::MemoryEventType event_type, ::llvm::Value* address, uint8_t width)
{
	::llvm::Function *fn = (::llvm::Function *)insn_ctx.block.region.txln.llvm_module->getOrInsertFunction("DefaultMemoryEventHandlerTranslatorRaiseEvent",
	                       insn_ctx.block.region.txln.types.vtype,
	                       insn_ctx.block.region.txln.types.cpu_ctx,
	                       insn_ctx.block.region.txln.types.pi8,
	                       insn_ctx.block.region.txln.types.i32,
	                       insn_ctx.block.region.txln.types.i32,
	                       insn_ctx.block.region.txln.types.i32,
	                       NULL);

	::llvm::Value *handler_ptr = insn_ctx.block.region.txln.GetConstantInt64((uint64_t)&handler);
	handler_ptr = insn_ctx.block.region.builder.CreateIntToPtr(handler_ptr, insn_ctx.block.region.txln.types.pi8);

	insn_ctx.block.region.builder.CreateCall5(fn,
	        insn_ctx.block.region.values.cpu_ctx_val,
	        handler_ptr,
	        insn_ctx.block.region.txln.GetConstantInt32(event_type),
	        address,
	        insn_ctx.block.region.txln.GetConstantInt32(width));

	return true;
}
