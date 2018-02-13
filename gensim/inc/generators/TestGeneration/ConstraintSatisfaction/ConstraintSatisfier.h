/*
 * ConstraintSatisfier.h
 *
 *  Created on: 25 Feb 2014
 *      Author: harry
 */

#ifndef CONSTRAINTSATISFIER_H_
#define CONSTRAINTSATISFIER_H_

#include <CoverageFile.h>
#include "generators/TestGeneration/ValueSet.h"
#include "generators/TestGeneration/ConstraintSet.h"

#include <unordered_set>

namespace gensim
{

	namespace arch
	{
		class ArchDescription;
	}

	namespace isa
	{
		class InstructionDescription;
	}

	namespace generator
	{
		namespace testgeneration
		{

			class ConstraintSatisfier
			{
			public:
				ConstraintSatisfier(const arch::ArchDescription &arch, const isa::InstructionDescription &insn);
				virtual ~ConstraintSatisfier();

				virtual void ProcessConstraints(const ConstraintSet &constraints, const coverage::CoveragePath &path) = 0;
				virtual bool IsSolved() = 0;
				virtual std::unordered_set<ValueSet> &GetSolutions() = 0;

			protected:
				const arch::ArchDescription &Arch;
				const isa::InstructionDescription &Insn;
			};

		}
	}
}

#endif /* CONSTRAINTSATISFIER_H_ */
