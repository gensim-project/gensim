/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
