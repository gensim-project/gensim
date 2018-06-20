/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"
#include "arch/MemoryInterfaceDescription.h"


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

				virtual bool IsFixed() const override;
				virtual bool Resolve(DiagnosticContext &ctx) override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;
				void Accept(SSAStatementVisitor& visitor) override;

				static SSAMemoryWriteStatement &CreateWrite(SSABlock *parent, SSAStatement *addrExpr, SSAStatement *valueExpr, uint8_t Width, const gensim::arch::MemoryInterfaceDescription *interface);

				~SSAMemoryWriteStatement();

				STATEMENT_OPERAND(Addr, 0);
				STATEMENT_OPERAND(Value, 1);

				const SSAType GetType() const override
				{
					return IRTypes::Void;
				}

				const gensim::arch::MemoryInterfaceDescription *GetInterface() const
				{
					return interface_;
				}

			private:
				SSAMemoryWriteStatement(SSABlock *parent, SSAStatement *addrExpr, SSAStatement *valueExpr, uint8_t width, const gensim::arch::MemoryInterfaceDescription *interface, SSAStatement *before = NULL)
					: SSAStatement(Class_Unknown, 2, parent, before), Width(width), interface_(interface)
				{
					SetAddr(addrExpr);
					SetValue(valueExpr);
				}

				const gensim::arch::MemoryInterfaceDescription *interface_;
			};
		}
	}
}
