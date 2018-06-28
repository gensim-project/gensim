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
			 * A statement which somehow overwrites the value stored in a variable
			 */
			class SSAVariableKillStatement : public SSAStatement
			{
			public:
				SSAVariableKillStatement(int extra_operands, SSABlock *_parent, SSASymbol *_target, SSAStatement *_before = NULL);
				virtual ~SSAVariableKillStatement();

				void Accept(SSAStatementVisitor& visitor) override;

				SYMBOL_OPERAND(Target, 0);

				const SSAType GetType() const override
				{
					return IRTypes::Void;
				}

			private:
				SSAVariableKillStatement() = delete;
			};
		}
	}
}
