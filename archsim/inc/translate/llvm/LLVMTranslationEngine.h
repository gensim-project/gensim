/*
 * File:   LLVMTranslationEngine.h
 * Author: s0457958
 *
 * Created on 07 August 2014, 15:54
 */

#ifndef LLVMTRANSLATIONENGINE_H
#define	LLVMTRANSLATIONENGINE_H

#include "translate/TranslationEngine.h"

namespace archsim
{
	namespace translate
	{
		namespace llvm
		{
			class LLVMTranslationEngine : public TranslationEngine
			{
			public:
				LLVMTranslationEngine() { }
				~LLVMTranslationEngine() { }

				bool Initialise() override;
				void Destroy() override;

				bool Translate(TranslationManager& mgr);
			};
		}
	}
}

#endif	/* LLVMTRANSLATIONENGINE_H */

