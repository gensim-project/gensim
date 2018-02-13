/*
 * genC/ssa/statement/SSAMemoryWriteStatement.h
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
			 * A statement representing a simulator memory write
			 */
			class SSAMemoryWriteStatement : public SSAStatement
			{
			public:
				uint8_t Width;
				bool User;

				virtual bool IsFixed() const override;
				virtual bool Resolve(DiagnosticContext &ctx) override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;
				void Accept(SSAStatementVisitor& visitor) override;

				static SSAMemoryWriteStatement &CreateWrite(SSABlock *parent, SSAStatement *addrExpr, SSAStatement *valueExpr, uint8_t Width, bool user = false);

				~SSAMemoryWriteStatement();

				STATEMENT_OPERAND(Addr, 0);
				STATEMENT_OPERAND(Value, 1);

				const SSAType GetType() const override
				{
					return IRTypes::Void;
				}

			private:
				SSAMemoryWriteStatement(SSABlock *parent, SSAStatement *addrExpr, SSAStatement *valueExpr, uint8_t width, bool user = false, SSAStatement *before = NULL)
					: SSAStatement(Class_Unknown, 2, parent, before), Width(width), User(user)
				{
					SetAddr(addrExpr);
					SetValue(valueExpr);
				}
			};
		}
	}
}