/*
 * genC/ssa/statement/SSARegisterStatement.h
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
			 * A statement representing a simulator register operation
			 */
			class SSARegisterStatement : public SSAStatement
			{
			public:
				bool IsRead;
				bool IsBanked;
				uint8_t Bank;

				virtual bool IsFixed() const override;
				void Accept(SSAStatementVisitor& visitor) override;

				virtual bool Resolve(DiagnosticContext &ctx) override;
				virtual void PrettyPrint(std::ostringstream &) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				static SSARegisterStatement *CreateBankedRead(SSABlock *parent, uint8_t bank, SSAStatement *regNumExpr);
				static SSARegisterStatement *CreateBankedWrite(SSABlock *parent, uint8_t bank, SSAStatement *regNumExpr, SSAStatement *valueExpr);

				static SSARegisterStatement *CreateRead(SSABlock *parent, uint8_t reg);
				static SSARegisterStatement *CreateWrite(SSABlock *parent, uint8_t reg, SSAStatement *valueExpr);

				~SSARegisterStatement();

				SSAStatement *RegNum()
				{
					return (SSAStatement*) GetOperand(0);
				}

				SSAStatement *Value()
				{
					return (SSAStatement*) GetOperand(1);
				}

				const SSAStatement *RegNum() const
				{
					return (SSAStatement*) GetOperand(0);
				}

				const SSAStatement *Value() const
				{
					return (SSAStatement*) GetOperand(1);
				}

				const SSAType GetType() const override
				{
					return IsRead ? ResolveType(*Parent->Parent->Arch, IsBanked, Bank) : IRTypes::Void;
				}

			private:
				SSARegisterStatement(SSABlock *parent, uint8_t bank, SSAStatement *regnumExpr, SSAStatement *valueExpr, SSAStatement *before = NULL)
					: SSAStatement(Class_Unknown, 2, parent, before), Bank(bank)
				{
					SetOperand(0, regnumExpr);
					SetOperand(1, valueExpr);
				}

				static SSAType ResolveType(const gensim::arch::ArchDescription& arch, bool is_banked, uint8_t bank);
			};
		}
	}
}
