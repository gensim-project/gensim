/*
 * File:   InterpretiveExecutionEngineGenerator.h
 * Author: s0803652
 *
 * Created on 14 October 2011, 14:34
 */

#ifndef _INTERPRETIVEEXECUTIONENGINEGENERATOR_H
#define _INTERPRETIVEEXECUTIONENGINEGENERATOR_H

#include <sstream>

#include "ExecutionEngineGenerator.h"
#include "GenerationManager.h"
#include "Util.h"

namespace gensim
{

	namespace isa
	{
		class ISADescription;
	}

	namespace generator
	{

		class InterpretiveExecutionEngineGenerator : public ExecutionEngineGenerator
		{
		public:
			bool Generate() const;

			InterpretiveExecutionEngineGenerator(GenerationManager &manager, std::string _name) : ExecutionEngineGenerator(manager, _name)
			{

			}

			InterpretiveExecutionEngineGenerator(GenerationManager &manager) : ExecutionEngineGenerator(manager, "interpret") {}

			const std::vector<std::string> GetSources() const;

			virtual ~InterpretiveExecutionEngineGenerator();

		protected:
			InterpretiveExecutionEngineGenerator(const InterpretiveExecutionEngineGenerator &orig);

			virtual bool GenerateHeader(util::cppformatstream &) const;
			virtual bool GenerateSource(util::cppformatstream &) const;

			virtual bool GenerateStepSingle(util::cppformatstream &) const;
			virtual bool GenerateStepSingleFast(util::cppformatstream &) const;
			virtual bool GenerateStepInstruction(util::cppformatstream &) const;
			virtual bool GenerateStepBlockFast(util::cppformatstream &) const;
			virtual bool GenerateStepBlockFastThread(util::cppformatstream &) const;
			virtual bool GenerateStepBlockTrace(util::cppformatstream &) const;
			bool GenerateThreadedDispatchCode(util::cppformatstream &stream) const;

			virtual bool GenerateInlineHelperFns(util::cppformatstream &) const;
			virtual bool GenerateExternHelperFns(util::cppformatstream &) const;

			virtual bool GenerateExecution(util::cppformatstream &, bool, std::string) const;
			virtual bool GenerateExecutionForBehaviour(util::cppformatstream &, bool, std::string, const isa::ISADescription &) const;

			virtual bool GenerateExtraProcessorClassMembers(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorSource(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorInitSource(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorDestructorSource(util::cppformatstream &) const;
			virtual bool GenerateExtraProcessorIncludes(util::cppformatstream &) const;

			virtual bool GenerateRegisterAccessFunctions(util::cppformatstream &) const;
			
			virtual bool GenerateModuleDescriptor(util::cppformatstream &) const;
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _INTERPRETIVEEXECUTIONENGINEGENERATOR_H */
