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
			 * A statement representing a conditional switch statement.
			 */
			class SSASwitchStatement : public SSAControlFlowStatement
			{
			public:
				typedef std::map<SSAStatement *, SSABlock *> ValueMap;

				virtual bool IsFixed() const override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				void AddValue(SSAStatement *expr, SSABlock *target)
				{
					AddOperand(expr);
					AddOperand(target);
				}

				virtual bool Resolve(DiagnosticContext& ctx) override;

				virtual target_list_t GetTargets() override;
				virtual target_const_list_t GetTargets() const override;

				void Accept(SSAStatementVisitor& visitor) override;

				STATEMENT_OPERAND(Expr, 0)
				BLOCK_OPERAND(Default, 1)

				SSASwitchStatement(SSABlock *parent, SSAStatement *expr, SSABlock *def, SSAStatement *before = NULL) : SSAControlFlowStatement(2, parent, before)
				{
					SetExpr(expr);
					SetDefault(def);
				}

				~SSASwitchStatement();

				const ValueMap GetValues() const;
			};
		}
	}
}
