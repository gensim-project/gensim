/*
 * genC/ssa/statement/SSASelectStatement.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{

			class SSASelectStatement : public SSAStatement
			{
			public:
				virtual bool IsFixed() const;
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();

				void Accept(SSAStatementVisitor& visitor) override;

				STATEMENT_OPERAND(Cond, 0)
				STATEMENT_OPERAND(TrueVal, 1)
				STATEMENT_OPERAND(FalseVal, 2)

				SSASelectStatement(SSABlock *parent, SSAStatement *_cond, SSAStatement *_if_true, SSAStatement *_if_false, SSAStatement *before = NULL)
					: SSAStatement(Class_Unknown, 3, parent, before)
				{
					SetOperand(0, _cond);
					SetOperand(1, _if_true);
					SetOperand(2, _if_false);
				}

				~SSASelectStatement() { }

				const SSAType GetType() const override
				{
					return TrueVal()->GetType();
				}
			};
		}
	}
}
