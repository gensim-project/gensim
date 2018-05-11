/*
 * File:   LLVMTranslation.h
 * Author: s0457958
 *
 * Created on 21 July 2014, 13:56
 */

#ifndef LLVMTRANSLATION_H
#define	LLVMTRANSLATION_H

#include "translate/Translation.h"
#include "util/PagePool.h"

namespace llvm
{
	class ExecutionEngine;
	class LLVMContext;
}

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace translate
	{
		namespace llvm
		{
			class LLVMMemoryManager;
			class LLVMTranslation : public Translation
			{
			public:
				using translation_fn = captive::shared::block_txln_fn;
				
				LLVMTranslation(translation_fn fnp, LLVMMemoryManager *mem_mgr);
				~LLVMTranslation();

				uint32_t Execute(archsim::core::thread::ThreadInstance* cpu);
				void Install(translation_fn *location) override;
				uint32_t GetCodeSize() const override;

			private:
				std::vector<util::PageReference*> zones;
				translation_fn fnp;
				uint32_t code_size;
			};
		}
	}
}

#endif	/* LLVMTRANSLATION_H */

