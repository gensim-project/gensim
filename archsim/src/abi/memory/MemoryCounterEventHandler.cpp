#include "abi/memory/MemoryCounterEventHandler.h"
#if CONFIG_LLVM
#include "translate/TranslationContext.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include <llvm/IR/IRBuilder.h>
#endif
#include "gensim/gensim_processor.h"

using namespace archsim::abi::memory;

bool MemoryCounterEventHandler::HandleEvent(gensim::Processor& cpu, MemoryModel::MemoryEventType type, guest_addr_t addr, uint8_t size)
{
	switch (type) {
		case MemoryModel::MemEventRead:
			mem_read.inc();
			break;
		case MemoryModel::MemEventFetch:
			mem_fetch.inc();
			break;
		case MemoryModel::MemEventWrite:
			mem_write.inc();
			break;
	}

	return true;
}

void MemoryCounterEventHandler::PrintStatistics(std::ostream& stream)
{
	stream << "Memory Event Statistics (% total instructions)" << std::endl;
	stream << "  Fetches: " << mem_fetch.get_value() << std::endl;
	stream << "    Reads: " << mem_read.get_value() << " (" << (((double)mem_read.get_value()/mem_fetch.get_value()) * 100) << ")" << std::endl;
	stream << "   Writes: " << mem_write.get_value() << " (" << (((double)mem_write.get_value()/mem_fetch.get_value()) * 100) << ")" << std::endl;
}

#if CONFIG_LLVM
bool MemoryCounterEventHandlerTranslator::EmitEventHandler(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, MemoryEventHandler& handler, MemoryModel::MemoryEventType event_type, ::llvm::Value *address, uint8_t width)
{
	MemoryCounterEventHandler& mce = (MemoryCounterEventHandler &)handler;

	switch(event_type) {
		case MemoryModel::MemEventFetch:
			return EmitCounterUpdate(insn_ctx, mce.mem_fetch, 1);
		case MemoryModel::MemEventRead:
			return EmitCounterUpdate(insn_ctx, mce.mem_read, 1);
		case MemoryModel::MemEventWrite:
			return EmitCounterUpdate(insn_ctx, mce.mem_write, 1);
		default:
			return false;
	}
}

bool MemoryCounterEventHandlerTranslator::EmitCounterUpdate(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, archsim::util::Counter64& counter, int64_t increment)
{
	auto& txln = insn_ctx.block.region.txln;
	auto& builder = insn_ctx.block.region.builder;

	uint64_t *const real_counter_ptr = counter.get_ptr();
	::llvm::Value *counter_ptr = builder.CreateIntToPtr(txln.GetConstantInt64((uint64_t)real_counter_ptr), txln.types.pi64, "counter_ptr");

	if (counter_ptr->getValueID() == ::llvm::Value::ConstantExprVal) {
		counter_ptr = ((::llvm::ConstantExpr *)counter_ptr)->getAsInstruction();
		builder.Insert((::llvm::Instruction *)counter_ptr);
	}

	txln.AddAliasAnalysisNode((::llvm::Instruction *)counter_ptr, archsim::translate::llvm::TAG_METRIC);
	::llvm::Value *loaded_val = builder.CreateLoad((::llvm::Instruction *)counter_ptr);
	::llvm::Value *added_val = builder.CreateAdd(loaded_val, txln.GetConstantInt64(increment));
	builder.CreateStore(added_val, counter_ptr);

	return true;
}
#endif
