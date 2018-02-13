/*
 * genC/ssa/statement/SSADeviceReadStatement.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ssa/statement/SSAVariableKillStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSADeviceReadStatement : public SSAVariableKillStatement
			{
			public:
				SSADeviceReadStatement(SSABlock *parent, SSAStatement *dev_expr, SSAStatement *addr_expr, SSASymbol *target);
				virtual ~SSADeviceReadStatement();

				virtual bool IsFixed() const override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				void Accept(SSAStatementVisitor& visitor) override;

				STATEMENT_OPERAND(Device, 1)
				STATEMENT_OPERAND(Address, 2)
			};
		}
	}
}
