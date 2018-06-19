/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSABitExtractStatement : public SSAStatement
			{
			public:
				STATEMENT_OPERAND(Expr, 0)
				STATEMENT_OPERAND(BitFrom, 1)
				STATEMENT_OPERAND(BitTo, 2)

				virtual void PrettyPrint(std::ostringstream &) const;
				void Accept(SSAStatementVisitor& visitor) override;

				bool IsFixed() const override;

				SSABitExtractStatement(SSABlock *parent, SSAStatement *Expr, SSAStatement* From, SSAStatement* To, SSAStatement *before = NULL);
				virtual ~SSABitExtractStatement() { }

				std::set<SSASymbol*> GetKilledVariables() override
				{
					return {};
				}

				const SSAType GetType() const override
				{
					return Expr()->GetType();
				}

			private:
				SSABitExtractStatement() = delete;
			};
		}
	}
}
