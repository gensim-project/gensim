/*
 * NullTranslationGenerator.h
 *
 *  Created on: Dec 23, 2012
 *      Author: harry
 */

#ifndef NULLTRANSLATIONGENERATOR_H_
#define NULLTRANSLATIONGENERATOR_H_

#include "GenerationManager.h"

#include <vector>
#include <string>

namespace gensim
{
	namespace generator
	{
		class NullTranslationGenerator : public GenerationComponent
		{
		public:
			NullTranslationGenerator(GenerationManager &man);

			virtual bool Generate() const;

			virtual const std::vector<std::string> GetSources() const;

			std::string GetFunction() const
			{
				return "translate";
			}
		};
	}
}

#endif /* NULLTRANSLATIONGENERATOR_H_ */
