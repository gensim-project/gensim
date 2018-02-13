/*
 * CVC4ConstraintFormatter.h
 *
 *  Created on: 24 Feb 2014
 *      Author: harry
 */

#ifndef CVC4CONSTRAINTFORMATTER_H_
#define CVC4CONSTRAINTFORMATTER_H_


#include <sstream>

namespace gensim
{

	namespace isa
	{
		class InstructionDescription;
	}
	namespace arch
	{
		class ArchDescription;
	}

	namespace generator
	{
		namespace testgeneration
		{

			class ConstraintExpression;
			class ConstraintBinaryExpression;
			class ConstraintSet;
			class ConstraintValue;
			class IConstraint;

			class CVC4ConstraintFormatter
			{
			public:
				CVC4ConstraintFormatter(const arch::ArchDescription& arch, const isa::InstructionDescription &insn, const ConstraintSet &constraints);

				std::string FormatConstraints();

				const arch::ArchDescription &Arch;
				const isa::InstructionDescription &Insn;
				const ConstraintSet &Constraints;
			private:
				void FormatValue(const ConstraintValue &value, std::ostringstream &str);
				void FormatExpression(const ConstraintExpression &expression, std::ostringstream &str);
				void FormatConstraint(const IConstraint &constraint, std::ostringstream &str);
				void FormatConstraintSet(const ConstraintSet& constraints, std::ostringstream &str);

				void FormatInfixExpression(const ConstraintBinaryExpression &expression, std::ostringstream &str);
				void FormatPrefixExpression(const ConstraintBinaryExpression &expression, std::ostringstream &str);
			};

		}
	}
}


#endif /* CVC4CONSTRAINTFORMATTER_H_ */
