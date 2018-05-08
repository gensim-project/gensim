/*
 * File:   MemoryCounterEventHandler.h
 * Author: s0457958
 *
 * Created on 25 August 2014, 15:45
 */

#ifndef MEMORYCOUNTEREVENTHANDLER_H
#define	MEMORYCOUNTEREVENTHANDLER_H

#include "abi/memory/MemoryEventHandler.h"
#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "util/Counter.h"

#include <ostream>

namespace archsim
{
	namespace util
	{
		class Counter64;
	}
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
#if CONFIG_LLVM
			class MemoryCounterEventHandlerTranslator : public MemoryEventHandlerTranslator
			{
			public:
				bool EmitEventHandler(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, MemoryEventHandler& handler, MemoryModel::MemoryEventType event_type, ::llvm::Value *address, uint8_t width) override;

			private:
				bool EmitCounterUpdate(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, archsim::util::Counter64& counter, int64_t increment);
			};
#endif

			class MemoryCounterEventHandler : public MemoryEventHandler
			{
				friend class MemoryCounterEventHandlerTranslator;
			public:
				bool HandleEvent(archsim::core::thread::ThreadInstance *cpu, MemoryModel::MemoryEventType type, Address addr, uint8_t size) override;

				void PrintStatistics(std::ostream& stream);

				MemoryEventHandlerTranslator& GetTranslator() override
				{
#if CONFIG_LLVM
					return translator;
#else
					assert(false);
#endif
				}

			private:
#if CONFIG_LLVM
				MemoryCounterEventHandlerTranslator translator;
#endif

				archsim::util::Counter64 mem_read;
				archsim::util::Counter64 mem_fetch;
				archsim::util::Counter64 mem_write;
			};
		}
	}
}

#endif	/* MEMORYCOUNTEREVENTHANDLER_H */

