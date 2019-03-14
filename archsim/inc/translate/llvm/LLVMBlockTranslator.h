/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef LLVMBLOCKTRANSLATOR_H__
#define LLVMBLOCKTRANSLATOR_H__

#include "gensim/gensim_translate.h"
#include "translate/TranslationWorkUnit.h"

#include <llvm/IR/Function.h>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMBlockTranslator
			{
			public:
				LLVMBlockTranslator(LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *txlt, llvm::Function *target_fn);

				llvm::BasicBlock *TranslateBlock(Address block_base, TranslationBlockUnit &block, llvm::BasicBlock *target_block);
			private:
				LLVMTranslationContext &ctx_;
				gensim::BaseLLVMTranslate *txlt_;
				llvm::Function *target_fn_;
			};

		}
	}
}

#endif
