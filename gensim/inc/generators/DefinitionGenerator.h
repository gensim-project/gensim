/*
 * File:   DefinitionGenerator.h
 * Author: s0803652
 *
 * Created on 07 October 2011, 15:50
 */

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
