/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAVectorShuffleStatement : public SSAStatement
			{
			public:
				SSAVectorShuffleStatement(SSABlock *parent, SSAStatement *lhs, SSAStatement *rhs, SSAStatement *shuffle, SSAStatement *before=nullptr);
				virtual ~SSAVectorShuffleStatement();

				virtual void PrettyPrint(std::ostringstream &) const override;

				virtual bool IsFixed() const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				void Accept(SSAStatementVisitor& visitor) override;

				STATEMENT_OPERAND(LHS, 0);
				STATEMENT_OPERAND(RHS, 1);
				STATEMENT_OPERAND(Indices, 2);

				const SSAType GetType() const override;

				static SSAVectorShuffleStatement *Concatenate(SSAStatement *lhs, SSAStatement *rhs);
			};
		}
	}
}
