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
			 * A statement representing a single operation on two genC registers, producing a single value.
			 */
			class SSABinaryArithmeticStatement : public SSAStatement
			{
			public:
				virtual bool IsFixed() const;
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();

				BinaryOperator::EBinaryOperator Type;

				virtual bool Resolve(DiagnosticContext &ctx) override;
				void Accept(SSAStatementVisitor& visitor) override;
				bool HasSideEffects() const override;

				STATEMENT_OPERAND(LHS, 0);
				STATEMENT_OPERAND(RHS, 1);

				SSABinaryArithmeticStatement(SSABlock *parent, SSAStatement *lhs, SSAStatement *rhs, BinaryOperator::EBinaryOperator type, SSAStatement *before = NULL)
					: SSAStatement(Class_Arithmetic, 2, parent, before),
					  Type(type)
				{
					SetLHS(lhs);
					SetRHS(rhs);
				}

				~SSABinaryArithmeticStatement();

				const SSAType GetType() const override
				{
					return IRType::Resolve(Type, LHS()->GetType(), RHS()->GetType());
				}

			private:
				SSABinaryArithmeticStatement();
			};
		}
	}
}
