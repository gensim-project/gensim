/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _TRANSLATIONGENERATOR_H
#define _TRANSLATIONGENERATOR_H

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "GenerationManager.h"
#include "Util.h"

#include <vector>

#define NOLIMM

namespace gensim
{
	namespace generator
	{

		class EmissionChunk
		{
		public:
			bool Quoted;
			std::string Text;

			EmissionChunk(bool quoted, std::string text) : Quoted(quoted), Text(text) {}

			static std::string Format(std::list<EmissionChunk> chunks);
			static std::list<EmissionChunk> Parse(std::string);
		};

		class TranslationGenerator : public GenerationComponent
		{
		public:
			std::string inline_inst(const isa::ISADescription &isa, std::string source, std::string object) const;

			std::string inline_macro(std::string source, std::string macro, std::vector<std::string>, bool trim_args) const;
			std::string inline_macro(std::string source, std::string macro, std::string output, bool trim_args) const;
			virtual std::string do_macros(const std::string code, bool trace) const;

			std::string format_code(const std::string source, const std::string, const bool trace) const;

			void emit_behaviour_fn(std::ofstream &str, const isa::ISADescription &isa, const std::string name, const std::string behaviour, bool trace, std::string ret_type) const;

			TranslationGenerator(GenerationManager &man, std::string name) : GenerationComponent(man, name) {};
			virtual ~TranslationGenerator();

			virtual bool GenerateHeader(std::ostringstream &target) const;
			bool GenerateJumpInfo(std::ostringstream &target) const;
			bool GenerateInstructionTranslations(std::ostringstream &target) const;

			virtual bool Generate() const;

			virtual const std::vector<std::string> GetSources() const;

			std::string GetFunction() const
			{
				return "translate";
			}

		private:
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _TRANSLATIONGENERATOR_H */
