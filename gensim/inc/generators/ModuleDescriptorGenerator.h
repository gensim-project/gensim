/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef MODULEDESCRIPTORGENERATOR_H
#define MODULEDESCRIPTORGENERATOR_H

#include "GenerationManager.h"

namespace gensim
{
	namespace generator
	{
		class ModuleDescriptorGenerator : public GenerationComponent
		{
		public:
			ModuleDescriptorGenerator(GenerationManager &man);

			bool Generate() const override;

			virtual const std::vector<std::string> GetSources() const;
			std::string GetFunction() const override;



		};
	}
}

#endif /* MODULEDESCRIPTORGENERATOR_H */

