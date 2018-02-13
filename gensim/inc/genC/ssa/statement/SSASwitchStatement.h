/*
 * genC/ssa/statement/SSASwitchStatement.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
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

				virtual bool IsFixed() const;
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();

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
