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
			 * A statement representing a read of a member of a struct
			 */
			class SSAReadStructMemberStatement : public SSAStatement
			{
			public:
				SYMBOL_OPERAND(Target, 0)

				std::vector<std::string> MemberNames;

				virtual bool IsFixed() const override;
				virtual bool Resolve(DiagnosticContext &ctx) override;
				bool HasSideEffects() const override;


				virtual void PrettyPrint(std::ostringstream &str) const override;
				virtual std::set<SSASymbol *> GetKilledVariables() override;

				SSAReadStructMemberStatement(SSABlock *parent, SSASymbol *target, const std::vector<std::string>& member, SSAStatement *before = NULL);

				void Accept(SSAStatementVisitor& visitor) override;

				const SSAType GetType() const override
				{
					return ResolveType(Target(), MemberNames);
				}

				std::string FormatMembers() const;

			private:
				static SSAType ResolveType(const SSASymbol *target, const std::vector<std::string>& member_name);
			};
		}
	}
}
