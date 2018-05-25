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
#include "translate/TranslationManager.h"
#include "blockjit/BlockJitTranslate.h"
#include "util/PagePool.h"

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
			
			uint8_t id;
			AsynchronousTranslationManager& mgr;
			volatile bool terminate;

			translate_llvm::LLVMOptimiser *optimiser;

			util::PagePool code_pool;

			void Translate(::llvm::LLVMContext& llvm_ctx, TranslationWorkUnit& unit);
		};
	}
}

#endif	/* ASYNCHRONOUSTRANSLATIONWORKER_H */

