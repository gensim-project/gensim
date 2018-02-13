/*
 * genC/ssa/statement/SSAVectorExtractElementStatement.h
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
			class SSAVectorExtractElementStatement : public SSAStatement
			{
			public:
				SSAVectorExtractElementStatement(SSABlock *parent, SSAStatement *base_expr, SSAStatement *index_expr);
				virtual ~SSAVectorExtractElementStatement();

				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;
				virtual bool IsFixed() const;
				bool HasSideEffects() const override;

				void Accept(SSAStatementVisitor& visitor) override;

				virtual bool Resolve(DiagnosticContext &ctx) override;

				STATEMENT_OPERAND(Base, 0)
				STATEMENT_OPERAND(Index, 1)

				const SSAType GetType() const override
				{
					return ResolveType(*Base());
				}

			private:
				const SSAType ResolveType(const SSAStatement& base) const;
			};
		}
	}
}
