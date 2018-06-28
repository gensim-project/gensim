/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

				virtual void PrettyPrint(std::ostringstream &) const override;
				void Accept(SSAStatementVisitor& visitor) override;
				bool Resolve(DiagnosticContext& ctx) override;

				SSAVariableReadStatement(SSABlock *parent, SSASymbol *Target, SSAStatement *before = NULL);
				virtual ~SSAVariableReadStatement();

				virtual bool IsFixed() const override
				{
					return Const;
				}

				virtual std::set<SSASymbol *> GetKilledVariables() override
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
