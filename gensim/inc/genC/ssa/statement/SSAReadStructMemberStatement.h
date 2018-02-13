/*
 * genC/ssa/statement/SSAReadStructMemoryStatement.h
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
			 * A statement representing a read of a member of a struct
			 */
			class SSAReadStructMemberStatement : public SSAStatement
			{
			public:
				SYMBOL_OPERAND(Target, 0)

				std::string MemberName;
				int32_t Index;

				virtual bool IsFixed() const;
				virtual bool Resolve(DiagnosticContext &ctx);
				bool HasSideEffects() const override;


				virtual void PrettyPrint(std::ostringstream &str) const;
				virtual std::set<SSASymbol *> GetKilledVariables();

				SSAReadStructMemberStatement(SSABlock *parent, SSASymbol *target, std::string member, int32_t idx = -1, SSAStatement *before = NULL);

				void Accept(SSAStatementVisitor& visitor) override;

				const SSAType GetType() const override
				{
					return ResolveType(Target(), MemberName, Index);
				}

			private:
				static SSAType ResolveType(const SSASymbol *target, const std::string& member_name, int index);
			};
		}
	}
}
