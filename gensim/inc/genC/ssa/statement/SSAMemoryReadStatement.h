/*
 * genC/ssa/statement/SSAMemoryReadStatement.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "arch/MemoryInterfaceDescription.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAMemoryReadStatement : public SSAVariableKillStatement
			{
			public:
				uint8_t Width;
				bool Signed;
				bool Fixed;

				virtual bool IsFixed() const override;
				virtual bool Resolve(DiagnosticContext &ctx) override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;
				void Accept(SSAStatementVisitor& visitor) override;
				
				gensim::arch::MemoryInterfaceDescription *GetInterface() {return interface_;}
				const gensim::arch::MemoryInterfaceDescription *GetInterface() const {return interface_;}

				static SSAMemoryReadStatement &CreateRead(SSABlock *parent, SSAStatement *addrExpr, SSASymbol *Target, uint8_t Width, bool sign, gensim::arch::MemoryInterfaceDescription *interface);

				~SSAMemoryReadStatement();

				STATEMENT_OPERAND(Addr, 1);

			private:
				SSAMemoryReadStatement(SSABlock *parent, SSAStatement *addrExpr, SSASymbol *target, uint8_t width, bool sign, gensim::arch::MemoryInterfaceDescription *interface)
					: SSAVariableKillStatement(1, parent, target), Width(width), Signed(sign), Fixed(false), interface_(interface)
				{
					SetAddr(addrExpr);
				}
					
				gensim::arch::MemoryInterfaceDescription *interface_;
			};
		}
	}
}
