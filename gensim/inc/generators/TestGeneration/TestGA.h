/*
 * TestGA.h
 *
 *  Created on: 3 Nov 2013
 *      Author: harry
 */

#ifndef TESTGA_H_
#define TESTGA_H_

#include <cstdint>
#include <memory>

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/TestGeneration/ValueSet.h"
#include "GAConfig.h"
#include "ConstraintSet.h"

namespace gensim
{
	namespace generator
	{
		namespace testgeneration
		{
			namespace ga
			{

				class GAGeneration
				{
				public:
					std::vector<ValueSet> Members;
				};

				class GAInstance
				{
				public:
					typedef std::mt19937 Random_t;

					GAInstance(const ConstraintSet &constraints_to_solve, const GAConfiguration &config);

					const std::unordered_set<ValueSet> &Run();

					GAGeneration *RunGeneration(const GAGeneration *input_generation);
					GAGeneration *CreateInitialGeneration();

					bool IsSolved() const;
					const std::unordered_set<ValueSet> &GetSolutions() const;

					void RunSelection(GAGeneration *out);
					void RunRecombination(const GAGeneration *in, GAGeneration *out);
					void RunMutation(GAGeneration *out);

					inline void SetLogFile(FILE *f)
					{
						_Logfile = f;
					}

				private:
					const GAConfiguration _Config;
					const ConstraintSet &_Constraints;

					Random_t _random;

					std::unordered_set<ValueSet> _Solutions;

					std::map<IConstraint *, uint64_t> _TotalConstraintFitness;
					std::map<IConstraint *, uint64_t> _FinalGenerationConstraintFitness;

					FILE *_Logfile;
				};
			}
		}
	}
}

#endif /* TESTGA_H_ */
