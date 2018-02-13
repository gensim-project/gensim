/*
 * CoverageCheckingInterpreterGenerator.h
 *
 *  Created on: 13 Sep 2013
 *      Author: harry
 */

#ifndef GENCINTERPRETERGENERATOR_H_
#define GENCINTERPRETERGENERATOR_H_

#include <unordered_set>
#include <fstream>

#include "generators/InterpretiveExecutionEngineGenerator.h"

namespace gensim
{
	namespace isa
	{
		class ISADescription;
	}

	namespace genc
	{
		namespace ssa
		{
			class SSAFormAction;
		}
	}

	namespace generator
	{

		class GenCInterpreterGenerator : public InterpretiveExecutionEngineGenerator
		{
		public:
			GenCInterpreterGenerator(GenerationManager &manager);

		protected:
			virtual bool GenerateExecutionForBehaviour(util::cppformatstream &, bool, std::string, const isa::ISADescription &) const;
			virtual bool GenerateExtraProcessorClassMembers(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorSource(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorInitSource(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorIncludes(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorDestructorSource(util::cppformatstream &stream) const;

			virtual bool GenerateInlineHelperFns(util::cppformatstream &) const override;
			virtual bool GenerateExternHelperFns(util::cppformatstream &) const override;

			bool GenerateExecuteBodyFor(util::cppformatstream &str, const genc::ssa::SSAFormAction &action) const;

		};
	}
}

#endif /* GENCINTERPRETERGENERATOR_H_ */
