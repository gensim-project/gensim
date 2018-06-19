/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _DEFINITIONGENERATOR_H
#define _DEFINITIONGENERATOR_H

#include "GenerationManager.h"

namespace gensim
{
	namespace generator
	{

		class DefinitionGenerator : public GenerationComponent
		{
		public:
			DefinitionGenerator(GenerationManager& man) : GenerationComponent(man, "define") {}

			bool Generate();

			std::string GetFunction() const
			{
				return "define";
			}

			virtual ~DefinitionGenerator();

		private:
			DefinitionGenerator(const DefinitionGenerator& orig);
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _DEFINITIONGENERATOR_H */
