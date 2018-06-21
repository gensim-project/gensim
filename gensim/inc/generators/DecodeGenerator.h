/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   DecodeGenerator.h
 * Author: mgould1
 *
 * Abstract superclass for decoder-generators. See FunctionalDecodeGenerator
 * and ClassfulDecodeGenerator for concrete implementations.
 *
 * Created on 04 May 2012, 17:18
 */

#ifndef _DECODEGENERATOR_H
#define _DECODEGENERATOR_H

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "GenerationManager.h"

#include <map>
#include <vector>

namespace gensim
{
	namespace generator
	{

		class DecodeNode;

		class DecodeGenerator : public GenerationComponent
		{
		public:
			DecodeGenerator(GenerationManager &man, std::string name) : GenerationComponent(man, name) {}
			virtual ~DecodeGenerator();

			const std::vector<std::string> GetSources() const;
			std::string GetFunction() const
			{
				return GenerationManager::FnDecode;
			}
			virtual void Setup(GenerationSetupManager &Setup);
			bool Generate() const;

			std::map<const isa::ISADescription *, DecodeNode *> decode_trees;

			virtual const std::string GetDecodeHeaderFilename() const
			{
				return "decode.h";
			}
			virtual const std::string GetDecodeSourceFilename() const
			{
				return "decode.cpp";
			}

		private:
			DecodeNode *BuildDecodeTree(GenerationSetupManager &Setup, const isa::ISADescription &isa);

		protected:
			typedef std::vector<uint32_t> SpecListType;
			typedef std::map<const isa::InstructionDescription *, SpecListType> SpecMapType;

			void DrawDotNode(const DecodeNode *tree, std::stringstream &str) const;
			bool GenerateDecodeDiagram(std::stringstream &str) const;
			bool GenerateDecodeEnum(util::cppformatstream &str) const;
			bool EmitInstructionFields(util::cppformatstream &stream) const;
			// override these to write a new decoder-generator.
			virtual bool GenerateDecodeHeader(util::cppformatstream &str) const = 0;
			virtual bool GenerateDecodeSource(util::cppformatstream &str) const = 0;

			SpecMapType specialisations;
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _DECODEGENERATOR_H */
