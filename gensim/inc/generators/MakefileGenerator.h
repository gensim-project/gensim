/*
 * File:   MakefileGenerator.h
 * Author: s0803652
 *
 * Created on 02 December 2011, 16:15
 */

#ifndef MAKEFILEGENERATOR_H
#define MAKEFILEGENERATOR_H

#include <vector>

#include "GenerationManager.h"
#include "clean_defines.h"

namespace gensim
{
	namespace generator
	{

		class MakefileGenerator : public GenerationComponent
		{
		public:
			MakefileGenerator(GenerationManager& man) : GenerationComponent(man, "makefile") {};
			virtual ~MakefileGenerator();

			bool Generate() const;
			void Reset();
			std::string GetFunction() const
			{
				return "make";
			}
			const std::vector<std::string> GetSources() const;

			void AddSource(std::string sourceName);
			void AddObjectFile(std::string objName);
			void AddPreBuildStep(std::string step);
			void AddPostBuildStep(std::string step);

		private:
			std::vector<std::string> sources;
			std::vector<std::string> objects;
			std::vector<std::string> preBuild;
			std::vector<std::string> postBuild;

			MakefileGenerator(const MakefileGenerator& orig);
		};

	}  // namespace generator
}  // namespace gensim

#endif /* MAKEFILEGENERATOR_H */
