/*
 * genC/ssa/statement/SSAVariableReadStatement.h
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
			/**
			 * A statement representing the read of a genC variable into a register
			 */
			class SSAVariableReadStatement : public SSAStatement
			{
			public:
				bool Const;

				SYMBOL_OPERAND(Target, 0)

				virtual void PrettyPrint(std::ostringstream &) const;
				void Accept(SSAStatementVisitor& visitor) override;
				bool Resolve(DiagnosticContext& ctx) override;

				SSAVariableReadStatement(SSABlock *parent, SSASymbol *Target, SSAStatement *before = NULL);
				virtual ~SSAVariableReadStatement();

				virtual bool IsFixed() const
				{
					return Const;
				}

				virtual std::set<SSASymbol *> GetKilledVariables()
				{
					return {};
				}

				bool HasSideEffects() const override
				{
					return false;
				}

				const SSAVariableWriteStatement *GetDominatingWriteFrom(const std::vector<SSABlock *> &block_list) const;
				const SSAVariableWriteStatement *GetFirstDominatingWrite() const;
				bool HasNoUndefinedPaths();

				const SSAType GetType() const override
				{
					return ResolveType(Target());
				}

			private:
				SSAVariableReadStatement() = delete;
				const SSAType ResolveType(const SSASymbol *target) const;
			};
		}
	}
}
