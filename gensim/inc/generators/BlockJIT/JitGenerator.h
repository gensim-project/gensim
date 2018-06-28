/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef JITGENERATOR_H
#define	JITGENERATOR_H

#include "generators/GenerationManager.h"
#include "Util.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ssa/SSAFormAction.h"

#include <vector>

namespace gensim
{
	namespace isa
	{
		class ISADescription;
		class InstructionDescription;
		class InstructionFormatDescription;
	}

	namespace generator
	{
		class JitNodeWalkerFactory : public genc::ssa::SSAWalkerFactory
		{
		public:
			virtual genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement);
		};

		class JitGenerator : public GenerationComponent
		{
		public:
			JitGenerator(GenerationManager &man);

			bool Generate() const override;
			const std::vector<std::string> GetSources() const override
			{
				return sources;
			}

			std::string GetFunction() const override
			{
				return "jit";
			}

		public:
			bool GenerateClass(util::cppformatstream &str) const;
			bool GenerateTranslation(util::cppformatstream &str) const;

			bool GenerateHeader(util::cppformatstream &) const;
			bool GenerateSource(util::cppformatstream &) const;

			mutable std::vector<std::string> sources;

			bool GeneratePredicateFunction(util::cppformatstream &, const isa::ISADescription& isa, const isa::InstructionFormatDescription& fmt) const;
			bool RegisterJITFunction(const isa::ISADescription& isa, const isa::InstructionDescription& insn) const;
			bool EmitJITFunction(util::cppformatstream &, const genc::ssa::SSAFormAction& action) const;

			bool GenerateJitChunks(int count) const;
			bool RegisterHelpers(const isa::ISADescription*) const;
		};
	}
}

#endif	/* JITGENERATOR_H */

