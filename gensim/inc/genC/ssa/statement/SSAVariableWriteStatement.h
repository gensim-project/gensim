/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAVariableKillStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			/**
			 * A statement representing a write of the value of a statement, to a variable.
			 * The entire value of the variable is replaced.
			 */
			class SSAVariableWriteStatement : public SSAVariableKillStatement
			{
			public:
				virtual bool IsFixed() const override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;
				void Accept(SSAStatementVisitor& visitor) override;

				virtual bool Resolve(DiagnosticContext &ctx) override;

				SSAVariableWriteStatement(SSABlock *_parent, SSASymbol *_target, SSAStatement *_expr, SSAStatement *_before = NULL);
				virtual ~SSAVariableWriteStatement();

				STATEMENT_OPERAND(Expr, 1);

			protected:
				SSAVariableWriteStatement();
			};
		}
	}
}
