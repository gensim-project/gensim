/*
 * ConstraintProcessor.h
 *
 *  Created on: 4 Nov 2013
 *      Author: harry
 */

#ifndef CONSTRAINTPROCESSOR_H_
#define CONSTRAINTPROCESSOR_H_

#include <unordered_set>

#include "generators/TestGeneration/Constraint.h"

namespace gensim
{
	namespace generator
	{
		namespace testgeneration
		{

			class ConstraintProcessor
			{
			public:
				virtual ~ConstraintProcessor();

				void ProcessConstraints(ConstraintSet &in_constraints);
				virtual void ProcessConstraint(const Constraint &in_constraint, ConstraintSet &out_constraints) = 0;
			};

			/**
			 * Class to expand out constraints of the form ((A (comp. op) B) != 0) into (A (comp. op) B)
			 */
			class NotEqualsConstraintProcessor : public ConstraintProcessor
			{
			public:
				void ProcessConstraint(const Constraint &in_constraint, ConstraintSet &out_constraints);
			};

			class CanonicalisationConstraintProcessor : public ConstraintProcessor
			{
			public:
				void ProcessConstraint(const Constraint &in_constraint, ConstraintSet &out_constraints);
			};

			class RemoveTrivialConstraintProcessor : public ConstraintProcessor
			{
			public:
				void ProcessConstraint(const Constraint &in_constraint, ConstraintSet &out_constraints);
			};
		}
	}
}

#endif /* CONSTRAINTPROCESSOR_H_ */
