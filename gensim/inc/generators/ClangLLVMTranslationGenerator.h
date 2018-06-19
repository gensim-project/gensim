/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _CLANGLLVMTRANSLATIONGENERATOR_H
#define _CLANGLLVMTRANSLATIONGENERATOR_H

#include "GenerationManager.h"
#include "TranslationGenerator.h"

#include "Util.h"

namespace gensim
{
	namespace isa
	{
		class InstructionDescription;
	}

	namespace generator
	{

		class ClangLLVMTranslationGenerator : public TranslationGenerator
		{
		public:
			ClangLLVMTranslationGenerator(GenerationManager &man) : TranslationGenerator(man, "translate_llvm") {}
			virtual ~ClangLLVMTranslationGenerator();

			bool Generate() const;
			void Setup(GenerationSetupManager &Setup);

			std::string GetFunction() const
			{
				return "translate";
			}

		private:
			ClangLLVMTranslationGenerator(const ClangLLVMTranslationGenerator &orig) = delete;
			bool GenerateSource(std::ostringstream &str) const;

			bool GenerateEmitPredicate(std::ostringstream &str) const;
			bool GenerateTranslateInstruction(std::ostringstream &str) const;

			bool GeneratePrecomp() const;

		};
	}
}

#endif /* _CLANGLLVMTRANSLATIONGENERATOR_H */
