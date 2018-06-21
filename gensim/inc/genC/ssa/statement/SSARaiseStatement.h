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
			 * A statement representing a genC return statement.
			 */
			class SSARaiseStatement : public SSAControlFlowStatement
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

				virtual target_list_t GetTargets() override
				{
					return {};
				}

				virtual target_const_list_t GetTargets() const override
				{
					return {};
				}

				SSARaiseStatement(SSABlock *parent, SSAStatement *before = NULL);
				~SSARaiseStatement();
			};
		}
	}
}
