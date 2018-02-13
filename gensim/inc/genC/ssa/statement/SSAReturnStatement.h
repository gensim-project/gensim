/*
 * genC/ssa/statement/SSAReturnStatement.h
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
			 * A statement representing a genC return statement.
			 */
			class SSAReturnStatement : public SSAControlFlowStatement
			{
			public:
				virtual bool IsFixed() const override;
				virtual bool Resolve(DiagnosticContext &ctx) override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				void Accept(SSAStatementVisitor& visitor) override;

				virtual bool ReplaceTarget(SSABlock *, SSABlock *)
				{
					return false;
				}

				virtual target_list_t GetTargets()
				{
					return {};
				}

				virtual target_const_list_t GetTargets() const
				{
					return {};
				}

				SSAStatement *Value()
				{
					return (SSAStatement*) GetOperand(0);
				}

				const SSAStatement *Value() const
				{
					return (SSAStatement*) GetOperand(0);
				}

				SSAReturnStatement(SSABlock *parent, SSAStatement *value, SSAStatement *before = NULL);

				~SSAReturnStatement();
			};
		}
	}
}
