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
#include "translate/llvm/LLVMCompiler.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "translate/llvm/LLVMTranslation.h"
#include "translate/TranslationManager.h"
#include "blockjit/BlockJitTranslate.h"
#include "util/PagePool.h"
#include "gensim/gensim_translate.h"


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
			AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::BaseLLVMTranslate *translate);

			void run();
			void stop();

			void PrintStatistics(std::ostream& stream);

		private:
			TranslationTimers timers;
			util::Counter64 units;
			util::Counter64 txlns;
			util::Counter64 blocks;

			gensim::BaseLLVMTranslate *translate_;

			uint8_t id;
			translate_llvm::LLVMCompiler compiler_;
			translate_llvm::LLVMOptimiser optimiser_;
			AsynchronousTranslationManager& mgr;
			volatile bool terminate;

			void Translate(::llvm::LLVMContext& llvm_ctx, TranslationWorkUnit& unit);
			translate_llvm::LLVMTranslation *CompileModule(TranslationWorkUnit& unit, ::llvm::Module *module, llvm::Function *function);
		};
	}
}

#endif	/* ASYNCHRONOUSTRANSLATIONWORKER_H */

