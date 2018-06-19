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
			 * Represents an operation which doesn't neatly fit into the other categories or should be
			 * implemented as a builtin/intrinsic operation
			 */
			class SSAIntrinsicStatement : public SSAStatement
			{
			public:
#define __DEFINE_INTRINSIC_ENUM
#include "genC/ssa/statement/SSAIntrinsicEnum.h"
#undef __DEFINE_INTRINSIC_ENUM

				IntrinsicType Type;

				SSAIntrinsicStatement(SSABlock *parent, IntrinsicType type, SSAStatement *before = NULL) : SSAStatement(Class_Unknown, 0, parent, before), Type(type) { }

				SSAStatement *Args(int idx)
				{
					return (SSAStatement*) GetOperand(idx);
				}

				const SSAStatement *Args(int idx) const
				{
					return (SSAStatement*) GetOperand(idx);
				}

				unsigned ArgCount() const
				{
					return OperandCount();
				}

				void AddArg(SSAStatement * const arg)
				{
					AddOperand(arg);
				}

				virtual bool IsFixed() const;

				virtual void PrettyPrint(std::ostringstream &) const;

				virtual std::set<SSASymbol *> GetKilledVariables();

				~SSAIntrinsicStatement();

				void Accept(SSAStatementVisitor& visitor) override;

				const SSAType GetType() const override
				{
					return ResolveType(Type);
				}

			private:
				const SSAType& ResolveType(IntrinsicType kind) const;
			};
		}
	}
}
