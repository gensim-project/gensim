/*
 * File:   LinkageGenerator.h
 * Author: s0803652
 *
 * Created on 19 October 2011, 14:58
 */

#ifndef _LINKAGEGENERATOR_H
#define _LINKAGEGENERATOR_H

#include "GenerationManager.h"

namespace gensim
{
	namespace generator
	{

		class LinkageGenerator : public GenerationComponent
		{
		public:
			LinkageGenerator(GenerationManager &man) : GenerationComponent(man, "linkage") {};
			virtual ~LinkageGenerator();

			bool Generate() const;

			std::string GetFunction() const
			{
				return "linkage";
			}

			const std::vector<std::string> GetSources() const;

		private:
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _LINKAGEGENERATOR_H */
