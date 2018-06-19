/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef CONSTRAINTSET_H_
#define CONSTRAINTSET_H_

#include <unordered_set>

#include "Constraint.h"

namespace gensim
{
	namespace generator
	{
		namespace testgeneration
		{

			class ConstraintSet : public IConstraint
			{
			public:
				typedef std::vector<IConstraint *> InnerConstraintSet_T;
				typedef InnerConstraintSet_T::iterator InnerConstraintSetIterator;
				typedef InnerConstraintSet_T::const_iterator InnerConstraintSetConstIterator;

				ConstraintSet();
				~ConstraintSet();

				//Deep copy
				ConstraintSet(const ConstraintSet &other);

				IConstraint *Clone() const;

				// Accessors for for(:) loops
				inline InnerConstraintSetIterator begin()
				{
					return _InnerConstraints.begin();
				}
				inline InnerConstraintSetConstIterator begin() const
				{
					return _InnerConstraints.begin();
				}

				inline InnerConstraintSetIterator end()
				{
					return _InnerConstraints.end();
				}
				inline InnerConstraintSetConstIterator end() const
				{
					return _InnerConstraints.end();
				}

				inline InnerConstraintSet_T::value_type operator[](int i)
				{
					return _InnerConstraints[i];
				}

				// Members inherited from IConstraint
				std::string prettyprint() const;
				uint64_t ScoreValueSet(const ValueSet &values) const;

				void GetFields(std::vector<std::string> &str) const;

				bool IsAtomic() const;
				bool IsSolvable() const;
				bool ConflictsWith(const IConstraint &other) const;

				void InsertConstraint(IConstraint *constraint);
				bool Equals(const IConstraint &other) const;

				inline size_t size() const
				{
					return _InnerConstraints.size();
				}

				enum {
					OP_And,
					OP_Or
				} Operator;

			private:
				InnerConstraintSet_T _InnerConstraints;
			};
		}
	}
}

#endif /* CONSTRAINTSET_H_ */
