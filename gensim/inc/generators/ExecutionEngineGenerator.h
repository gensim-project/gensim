/*
 * ExecutionEngineGenerator.h
 *
 *  Created on: Oct 31, 2012
 *      Author: harry
 */

#ifndef EXECUTIONENGINEGENERATOR_H_
#define EXECUTIONENGINEGENERATOR_H_

#include "GenerationManager.h"

namespace gensim
{
	namespace generator
	{
		class ExecutionEngineGenerator : public GenerationComponent
		{
		public:
			virtual bool Generate() const = 0;

			std::string GetFunction() const
			{
				return GenerationManager::FnInterpret;
			}

			ExecutionEngineGenerator(const GenerationManager &manager, std::string _name) : GenerationComponent(manager, _name) {}

			std::string GetStateStruct() const;

			virtual const std::vector<std::string> GetSources() const = 0;

			virtual ~ExecutionEngineGenerator();
		};
	}
}

#endif /* EXECUTIONENGINEGENERATOR_H_ */
