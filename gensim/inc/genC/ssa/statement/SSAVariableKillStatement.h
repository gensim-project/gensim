/*
 * genC/ssa/statement/SSAVariableKillStatement.h
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
