/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef YICESCONSTRAINTFORMATTER_H_
#define YICESCONSTRAINTFORMATTER_H_

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
			class ConstraintSet;
			class ConstraintValue;
			class IConstraint;

			class YicesConstraintFormatter
			{
			public:
				YicesConstraintFormatter(const arch::ArchDescription& arch, const isa::InstructionDescription &insn, const ConstraintSet &constraints);

				std::string FormatConstraints();

				const arch::ArchDescription &Arch;
				const isa::InstructionDescription &Insn;
				const ConstraintSet &Constraints;
			private:
				void FormatValue(const ConstraintValue &value, std::ostringstream &str);
				void FormatExpression(const ConstraintExpression &expression, std::ostringstream &str);
				void FormatConstraint(const IConstraint &constraint, std::ostringstream &str);
				void FormatConstraintSet(const ConstraintSet& constraints, std::ostringstream &str);
			};

		}
	}
}

#endif /* YICESCONSTRAINTFORMATTER_H_ */
