/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include "genC/ssa/statement/SSAStatement.h"
#include "util/MaybeVector.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAControlFlowStatement : public SSAStatement
			{
			public:
				virtual void Accept(SSAStatementVisitor& visitor) override;

				typedef util::MaybeVector<SSABlock*, 2> target_list_t;
				typedef util::MaybeVector<const SSABlock*, 2> target_const_list_t;

				virtual target_list_t GetTargets() = 0;
				virtual target_const_list_t GetTargets() const = 0;

				void Move(SSABlock *newParent, SSAStatement *insertBefore = NULL);

				SSAControlFlowStatement(int operands, SSABlock *parent, SSAStatement *before = NULL) : SSAStatement(Class_Controlflow, operands, parent, before) { }

				const SSAType GetType() const override
				{
					return IRTypes::Void;
				}
			};
		}
	}
}
