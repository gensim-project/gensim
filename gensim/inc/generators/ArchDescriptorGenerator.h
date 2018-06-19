/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef ARCHDESCRIPTORGENERATOR_H
#define ARCHDESCRIPTORGENERATOR_H

#include "GenerationManager.h"
#include "Util.h"

namespace gensim
{
	namespace generator
	{

		class ArchDescriptorGenerator : public GenerationComponent {
		public:
			ArchDescriptorGenerator(GenerationManager &manager);
			
			bool Generate() const override;
			std::string GetFunction() const override;
			const std::vector<std::string> GetSources() const override;
			
		private:
			bool GenerateHeader(util::cppformatstream &str) const;
			bool GenerateSource(util::cppformatstream &str) const;
			
			bool GenerateThreadInterface(util::cppformatstream &str) const;
		};
		
	}
}

#endif /* ARCHDESCRIPTORGENERATOR_H */

