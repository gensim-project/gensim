/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAVectorInsertElementStatement : public SSAStatement
			{
			public:
				SSAVectorInsertElementStatement(SSABlock *parent, SSAStatement *initial, SSAStatement *index_expr, SSAStatement *value_expr);
				virtual ~SSAVectorInsertElementStatement();

				virtual void PrettyPrint(std::ostringstream &) const override;

				virtual bool IsFixed() const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				void Accept(SSAStatementVisitor& visitor) override;

				STATEMENT_OPERAND(Index, 0)
				STATEMENT_OPERAND(Value, 1)
				STATEMENT_OPERAND(Base, 2)

				const SSAType GetType() const override
				{
					return Base()->GetType();
				}
			};
		}
	}
}
