/*
 * File:   InsnJITGenerator.h
 * Author: s0457958
 *
 * Created on 28 April 2014, 13:46
 */

#ifndef INSNJITGENERATOR_H
#define	INSNJITGENERATOR_H

#include "generators/GenerationManager.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"
#include "genC/ssa/SSAWalkerFactory.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSANodeWalker;
			class SSAStatement;
			class SSAFormAction;
		}
	}

	namespace isa
	{
		class InstructionDescription;
	}
	namespace util
	{
		class cppformatstream;
	}
	namespace generator
	{

		namespace insnjit
		{
			class LIRNodeWalkerFactory : public genc::ssa::SSAWalkerFactory
			{
			public:
				virtual genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement);
			};
		}

		class InsnJITGenerator : public InterpretiveExecutionEngineGenerator
		{
		public:
			InsnJITGenerator(GenerationManager &manager);
			~InsnJITGenerator();

		protected:
			virtual bool GenerateRunNotrace(util::cppformatstream &) const;
			virtual bool GenerateRunTrace(util::cppformatstream &) const;
			virtual bool GenerateStepSingle(util::cppformatstream &) const;
			virtual bool GenerateStepSingleFast(util::cppformatstream &) const;
			virtual bool GenerateStepBlockFast(util::cppformatstream &) const;
			virtual bool GenerateStepBlockTrace(util::cppformatstream &) const;

			virtual bool GenerateExtraProcessorClassMembers(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorSource(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorInitSource(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorDestructorSource(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorIncludes(util::cppformatstream &) const;

		private:
			bool GenerateEmitterSource(util::cppformatstream &output, const gensim::genc::ssa::SSAFormAction& insn) const;
		};

	}
}

#endif	/* INSNJITGENERATOR_H */

