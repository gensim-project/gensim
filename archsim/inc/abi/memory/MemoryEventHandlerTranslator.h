/*
 * File:   MemoryEventHandlerTranslator.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 10:08
 */

#ifndef MEMORYEVENTHANDLERTRANSLATOR_H
#define	MEMORYEVENTHANDLERTRANSLATOR_H

#include "abi/memory/MemoryModel.h"
#include "abi/memory/MemoryEventHandler.h"

namespace llvm
{
	class Value;
}

namespace archsim
{
	namespace translate
	{
		namespace llvm
		{
			class LLVMInstructionTranslationContext;
		}
	}

	namespace abi
	{
		namespace memory
		{
			class MemoryEventHandlerTranslator
			{
			public:
				virtual bool EmitEventHandler(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, MemoryEventHandler& handler, MemoryModel::MemoryEventType event_type, ::llvm::Value *address, uint8_t width) = 0;
			};

			class DefaultMemoryEventHandlerTranslator : public MemoryEventHandlerTranslator
			{
			public:
				bool EmitEventHandler(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, MemoryEventHandler& handler, MemoryModel::MemoryEventType event_type, ::llvm::Value *address, uint8_t width) override;
			};
		}
	}
}

#endif	/* MEMORYEVENTHANDLERTRANSLATOR_H */

