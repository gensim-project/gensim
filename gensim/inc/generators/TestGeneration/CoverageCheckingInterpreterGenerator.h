/*
 * CoverageCheckingInterpreterGenerator.h
 *
 *  Created on: 13 Sep 2013
 *      Author: harry
 */

#ifndef COVERAGECHECKINGINTERPRETERGENERATOR_H_
#define COVERAGECHECKINGINTERPRETERGENERATOR_H_

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

		class CoverageCheckingInterpreterGenerator : public InterpretiveExecutionEngineGenerator
		{
		public:
			CoverageCheckingInterpreterGenerator(GenerationManager &manager);

		protected:
			virtual bool GenerateExecutionForBehaviour(util::cppformatstream &, bool, std::string, const isa::ISADescription &) const;
			virtual bool GenerateExtraProcessorClassMembers(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorSource(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorInitSource(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorIncludes(util::cppformatstream &stream) const;
			virtual bool GenerateExtraProcessorDestructorSource(util::cppformatstream &stream) const;

			bool GenerateExecuteBodyFor(util::cppformatstream &str, const genc::ssa::SSAFormAction &action) const;

		private:
			mutable std::ofstream output_file;
			mutable std::map<std::string, uint32_t> action_ids;
			mutable std::unordered_set<std::string> emitted_actions;
			inline uint32_t GetOrCreateID(std::string behaviour) const
			{
				if (action_ids.find(behaviour) != action_ids.end()) return action_ids[behaviour];
				action_ids[behaviour] = action_ids.size();
				return action_ids.at(behaviour);
			}
		};
	}
}

#endif /* COVERAGECHECKINGINTERPRETERGENERATOR_H_ */
