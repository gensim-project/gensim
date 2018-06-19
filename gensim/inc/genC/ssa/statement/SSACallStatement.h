/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{

			/**
			 * A statement representing a genC call to a helper action. Calls are always 'inlined'.
			 */
			class SSACallStatement : public SSAStatement
			{
			public:
				ACTION_OPERAND(Target, 0)

				virtual bool IsFixed() const;
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();
				void Accept(SSAStatementVisitor& visitor) override;

				unsigned ArgCount() const
				{
					return OperandCount() - 1;
				}

				SSAValue *Arg(unsigned i)
				{
					return (SSAValue*) GetOperand(i + 1);
				}

				const SSAValue *Arg(unsigned i) const
				{
					return (SSAValue*) GetOperand(i + 1);
				}

				SSACallStatement(SSABlock *parent, SSAActionBase *target, std::vector<SSAValue *> args);
				~SSACallStatement();

				const SSAType GetType() const override
				{
					return Target()->GetPrototype().ReturnType();
				}

			private:
				bool IsFullyConst;
			};

		}
	}
}
