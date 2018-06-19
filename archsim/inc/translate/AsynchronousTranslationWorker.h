/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   AsynchronousTranslationWorker.h
 * Author: s0457958
 *
 * Created on 18 July 2014, 14:56
 */

#ifndef ASYNCHRONOUSTRANSLATIONWORKER_H
#define	ASYNCHRONOUSTRANSLATIONWORKER_H

#include "concurrent/Thread.h"
#include "util/Counter.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "translate/llvm/LLVMTranslation.h"
#include "translate/TranslationManager.h"
#include "blockjit/BlockJitTranslate.h"
#include "util/PagePool.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

namespace llvm
{
	class LLVMContext;
}

namespace archsim
{
	namespace translate
	{
		class AsynchronousTranslationManager;
		class TranslationWorkUnit;

		class AsynchronousTranslationWorker : public concurrent::Thread
		{
			friend class AsynchronousTranslationManager;
		public:
			AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::blockjit::BaseBlockJITTranslate *translate);

			void run();
			void stop();

			void PrintStatistics(std::ostream& stream);

		private:
			TranslationTimers timers;
			util::Counter64 units;
			util::Counter64 txlns;
			util::Counter64 blocks;

			gensim::blockjit::BaseBlockJITTranslate *translate_;
			
			std::shared_ptr<llvm::RTDyldMemoryManager> memory_manager_;
			std::unique_ptr<llvm::TargetMachine> target_machine_;
			llvm::orc::RTDyldObjectLinkingLayer linker_;
			llvm::orc::IRCompileLayer<decltype(linker_), llvm::orc::SimpleCompiler> compiler_;
			
			uint8_t id;
			AsynchronousTranslationManager& mgr;
			volatile bool terminate;

			translate_llvm::LLVMOptimiser *optimiser;

			util::PagePool code_pool;

			void Translate(::llvm::LLVMContext& llvm_ctx, TranslationWorkUnit& unit);
			translate_llvm::LLVMTranslation *CompileModule(TranslationWorkUnit& unit, ::llvm::Module *module, llvm::Function *function);
		};
	}
}

#endif	/* ASYNCHRONOUSTRANSLATIONWORKER_H */

