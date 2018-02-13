/*
 * File:   ClangDynamicGenerator.h
 * Author: s0803652
 *
 * Created on 15 March 2012, 18:27
 */

#ifndef CLANGDYNAMICGENERATOR_H
#define CLANGDYNAMICGENERATOR_H

#include "GenerationManager.h"

namespace gensim
{
	namespace generator
	{

		class ClangDynamicGenerator : GenerationComponent
		{
		public:
			ClangDynamicGenerator(GenerationManager&);
			ClangDynamicGenerator(const ClangDynamicGenerator& orig);
			virtual ~ClangDynamicGenerator();

			bool Generate() const;
			void Reset();
			void Setup(GenerationSetupManager& Setup);
			std::string GetFunction() const;

			const std::vector<std::string> GetSources() const;

		private:
		};

	}  // namespace generator
}  // namespace gensim

#endif /* CLANGDYNAMICGENERATOR_H */
