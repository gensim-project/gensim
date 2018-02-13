/*
 * NullTranslationGenerator.cpp
 *
 *  Created on: Dec 23, 2012
 *      Author: harry
 */

#include "arch/ArchDescription.h"
#include "generators/NullTranslationGenerator.h"

DEFINE_COMPONENT(gensim::generator::NullTranslationGenerator, NullTranslationGenerator);

namespace gensim
{
	namespace generator
	{

		NullTranslationGenerator::NullTranslationGenerator(GenerationManager& man) : GenerationComponent(man, "translate") {}

		const std::vector<std::string> NullTranslationGenerator::GetSources() const
		{
			std::vector<std::string> sources;
			sources.push_back("translate.cpp");
			return sources;
		}

		bool NullTranslationGenerator::Generate() const
		{
			std::string headerStr = Manager.GetTarget();
			headerStr.append("/translate.h");

			std::string sourceStr = Manager.GetTarget();
			sourceStr.append("/translate.cpp");

			std::ofstream headerStream(headerStr.c_str());
			std::ofstream sourceStream(sourceStr.c_str());

			util::cppformatstream header;
			util::cppformatstream source;
			header << ""
			       "#ifndef __HEADER_TRANSLATE_" << Manager.GetArch().Name << "\n"
			       "#define __HEADER_TRANSLATE_" << Manager.GetArch().Name << "\n"
			       "#include <gensim/gensim_translate.h>\n"
			       "#include <string>\n"
			       "namespace llvm { class LLVMContext; }"
			       "namespace gensim { "
			       "class Processor;\n "
			       "class BaseDecode;\n"
			       "namespace " << Manager.GetArch().Name << "{ \n"
			       "class Translate : public gensim::BaseTranslate{"
			       "public: "
			       "Translate(gensim::Processor &cpu);"
			       "Translate(gensim::Processor &cpu, llvm::LLVMContext &ctx);"
			       "};"
			       "}}\n"
			       "#endif";

			source << "#include \"translate.h\"\n"
			       "#include <cassert>\n"
			       "namespace gensim {"
			       "namespace " << Manager.GetArch().Name << "{ "
			       "Translate::Translate(gensim::Processor &cpu) { assert(false && \"This processor model was not built with JIT support\");}"
			       "Translate::Translate(gensim::Processor &cpu, llvm::LLVMContext &ctx) {assert(false && \"This processor model was not built with JIT support\");}"
			       "}}";

			headerStream << header.str();
			sourceStream << source.str();

			return true;
		}
	}
}
