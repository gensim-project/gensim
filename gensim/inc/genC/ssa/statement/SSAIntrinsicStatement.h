/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ir/IRSignature.h"

#include <functional>

namespace gensim
{
	namespace genc
	{
		class IntrinsicDescriptor;

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

				SSAIntrinsicStatement(SSABlock *parent, const gensim::genc::IntrinsicDescriptor &descriptor, const SSAType &return_type, SSAStatement *before = NULL)
					: SSAStatement(Class_Unknown, 0, parent, before), descriptor_(descriptor), return_type_(return_type) { }

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
					return return_type_;
				}

				const IntrinsicDescriptor &GetDescriptor() const { return descriptor_; }

				virtual bool IsFixed() const override;

			private:
				const IntrinsicDescriptor &descriptor_;
				SSAType return_type_;
			};
		}
	}
}
