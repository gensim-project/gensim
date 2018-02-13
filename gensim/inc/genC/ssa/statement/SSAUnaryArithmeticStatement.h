/*
 * genC/ssa/statement/SSAUnaryArithmeticStatement.h
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
			 * A statement representing a single simple operation on one genC register, producing a value.
			 */
			class SSAUnaryArithmeticStatement : public SSAStatement
			{
			public:
				virtual bool IsFixed() const;
				virtual bool Resolve(DiagnosticContext &ctx);
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();
				bool HasSideEffects() const override;

				void Accept(SSAStatementVisitor& visitor) override;

				STATEMENT_OPERAND(Expr, 0)

				SSAUnaryOperator::ESSAUnaryOperator Type;

				~SSAUnaryArithmeticStatement();

				SSAUnaryArithmeticStatement(SSABlock *parent, SSAStatement *expr, SSAUnaryOperator::ESSAUnaryOperator type, SSAStatement *before = NULL)
					: SSAStatement(Class_Arithmetic, 1, parent, before), Type(type)
				{
					SetOperand(0, expr);
				}

				const SSAType GetType() const override
				{
					return Expr()->GetType();
				}

			private:
				SSAUnaryArithmeticStatement();
			};
		}
	}
}
