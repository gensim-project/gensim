/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SynchronousTranslationManager.h
 * Author: s0457958
 *
 * Created on 06 August 2014, 09:21
 */

#ifndef SYNCHRONOUSTRANSLATIONMANAGER_H
#define	SYNCHRONOUSTRANSLATIONMANAGER_H

#include "gensim/gensim_translate.h"
#include "translate/TranslationManager.h"
#include "translate/llvm/LLVMCompiler.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "util/PagePool.h"

namespace archsim
{
	namespace translate
	{
		namespace profile
		{
			class Region;
		}

		class TranslationEngine;
		class SynchronousTranslationManager : public TranslationManager
		{
		public:
			SynchronousTranslationManager(util::PubSubContext *psctx);
			~SynchronousTranslationManager();

			bool Initialise(gensim::BaseLLVMTranslate *translator);
			void Destroy() override;

			bool TranslateRegion(archsim::core::thread::ThreadInstance *thread, profile::Region& rgn, uint32_t weight);

		private:
			TranslationTimers timers;

			gensim::BaseLLVMTranslate *translator_;
			translate_llvm::LLVMOptimiser optimiser_;
			translate_llvm::LLVMCompiler compiler_;

			util::PagePool code_pool;
		};

	}
}

#endif	/* SYNCHRONOUSTRANSLATIONMANAGER_H */

