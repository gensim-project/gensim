/*
 * File:   DisasmGenerator.h
 * Author: s0803652
 *
 * Created on 30 September 2011, 12:28
 */

#ifndef _DISASMGENERATOR_H
#define _DISASMGENERATOR_H

#include "arch/ArchDescription.h"
#include "GenerationManager.h"

namespace gensim
{

	namespace isa
	{
		class AsmDescription;
		class InstructionDescription;
	}

	namespace generator
	{

		class AsmConstraintComparison
		{
			bool rev;

		public:
			AsmConstraintComparison() : rev(false) {}

			AsmConstraintComparison(bool _rev) : rev(_rev) {};
			bool operator()(const isa::AsmDescription* const &a, const isa::AsmDescription* const &b) const;
		};

		class DisasmGenerator : public GenerationComponent
		{
		public:
			DisasmGenerator(GenerationManager &man) : GenerationComponent(man, "disasm") {}

			std::string GetFunction() const
			{
				return GenerationManager::FnDisasm;
			}
			bool Generate() const;

			const std::vector<std::string> GetSources() const;

			virtual ~DisasmGenerator();

		private:
			bool GenerateDisasm(std::string prefix) const;

			bool GenerateInstrDisasm(std::stringstream &source_str, const isa::ISADescription &isa, const isa::InstructionDescription &instr) const;

			bool GenerateDisasmHeader(std::stringstream &header_str) const;
			bool GenerateDisasmSource(std::stringstream &source_str) const;

			DisasmGenerator(const DisasmGenerator &orig);
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _DISASMGENERATOR_H */
