/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ir/IRSignature.h"
#include "genC/Intrinsics.h"

#include <functional>

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
				using SSAFixednessResolver = std::function<bool(const SSAIntrinsicStatement *)>;

				SSAIntrinsicStatement(SSABlock *parent, IntrinsicID id, const IRSignature& signature, SSAFixednessResolver fixednessResolver, SSAStatement *before = NULL)
					: SSAStatement(Class_Unknown, 0, parent, before), id_(id), signature_(signature), fixedness_resolver_(fixednessResolver) { }

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

				virtual void PrettyPrint(std::ostringstream &) const override;

				virtual std::set<SSASymbol *> GetKilledVariables() override;

				~SSAIntrinsicStatement();

				void Accept(SSAStatementVisitor& visitor) override;

				const SSAType GetType() const override
				{
					return signature_.GetType();
				}

				virtual bool IsFixed() const override
				{
					return fixedness_resolver_(this);
				}

				IntrinsicID GetID() const
				{
					return id_;
				}
				const IRSignature& GetSignature() const
				{
					return signature_;
				}

				SSAFixednessResolver GetFixednessResolverFunction() const
				{
					return fixedness_resolver_;
				}

			private:
				IntrinsicID id_;
				const IRSignature signature_;
				SSAFixednessResolver fixedness_resolver_;
			};
		}
	}
}
