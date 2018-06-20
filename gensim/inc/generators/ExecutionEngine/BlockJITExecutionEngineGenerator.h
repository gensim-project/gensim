/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef BLOCKJITEXECUTIONENGINEGENERATOR_H
#define BLOCKJITEXECUTIONENGINEGENERATOR_H

#include "generators/GenerationManager.h"
#include "generators/ExecutionEngine/EEGenerator.h"

namespace gensim
{
	namespace generator
	{
		class BlockJITExecutionEngineGenerator : public EEGenerator
		{
		public:
			BlockJITExecutionEngineGenerator(GenerationManager &man);

			bool GenerateHeader(util::cppformatstream& str) const override;
			bool GenerateSource(util::cppformatstream& str) const override;
			const std::vector<std::string> GetSources() const override;
			void Setup(GenerationSetupManager& Setup) override;


		private:
			mutable std::vector<std::string> sources;
		};
	}
}

#endif /* BLOCKJITEXECUTIONENGINEGENERATOR_H */

