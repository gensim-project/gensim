/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAControlFlowStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			/**
			 * A statement representing a conditional branch in genC control flow.
			 */
			class SSAIfStatement : public SSAControlFlowStatement
			{
			public:
				virtual bool IsFixed() const override;
				virtual bool Resolve(DiagnosticContext &ctx) override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;
				void Accept(SSAStatementVisitor& visitor) override;

				virtual target_list_t GetTargets() override;
				virtual target_const_list_t GetTargets() const override;

				STATEMENT_OPERAND(Expr, 0)
				BLOCK_OPERAND(TrueTarget, 1)
				BLOCK_OPERAND(FalseTarget, 2)

				SSAIfStatement(SSABlock *parent, SSAStatement *expr, SSABlock &trueTarget, SSABlock &falseTarget, SSAStatement *before = NULL)
					: SSAControlFlowStatement(3, parent, before)
				{
					SetExpr(expr);
					SetTrueTarget(&trueTarget);
					SetFalseTarget(&falseTarget);
				}

				virtual ~SSAIfStatement()
				{
					if (TrueTarget()) TrueTarget()->RemovePredecessor(*Parent);
					if (FalseTarget()) FalseTarget()->RemovePredecessor(*Parent);
				}
			};
		}
	}
}
