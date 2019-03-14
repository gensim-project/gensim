/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef LLVMWORKUNITTRANSLATOR_H__
#define LLVMWORKUNITTRANSLATOR_H__

#include "translate/TranslationWorkUnit.h"

#include <llvm/IR/LLVMContext.h>

namespace llvm
{
	class Module;
	class TargetMachine;
}

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{
			class LLVMWorkUnitTranslator
			{
			public:
				LLVMWorkUnitTranslator(gensim::BaseLLVMTranslate *txlt, llvm::LLVMContext &ctx);

				std::pair<llvm::Module*, llvm::Function*> TranslateWorkUnit(TranslationWorkUnit &twu);

			private:
				llvm::LLVMContext &llvm_ctx_;
				std::unique_ptr<llvm::TargetMachine> target_machine_;

				gensim::BaseLLVMTranslate *translate_;

			};
		}
	}
}

#endif
