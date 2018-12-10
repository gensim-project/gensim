/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/GenerationManager.h"

#include <vector>

namespace gensim
{
	namespace generator
	{
		class LLVMBCBuilderGenerator : public GenerationComponent
		{

			virtual ~LLVMBCBuilderGenerator()
			{

			}

			virtual bool Generate() const
			{

				return false;
			}

			virtual std::string GetFunction() const
			{
				return "LLVMBCBuilder";
			}

			virtual const std::vector<std::string> GetSources() const
			{
				return {};
			}

			virtual void Reset() {}

			virtual void Setup(GenerationSetupManager& Setup) {}

		};
	}
}