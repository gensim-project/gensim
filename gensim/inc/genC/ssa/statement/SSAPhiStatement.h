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
			 * A Phi node
			 */
			class SSAPhiStatement : public SSAStatement
			{
			public:
				typedef std::set<SSAStatement*> member_set_t;
				typedef std::set<std::string>   placeholder_set_t;
				SSAPhiStatement(SSABlock *parent, const SSAType &type, SSAStatement *before = nullptr);
				virtual ~SSAPhiStatement();

				bool Add(SSAStatement *member);
				bool AddPlaceholder(const std::string &placeholder);
				const operand_list_t &Get() const;
				void Set(const operand_list_t &ops);
				unsigned int Count() const;

				virtual bool IsFixed() const override;
				bool HasSideEffects() const override;

				std::set<SSASymbol*> GetKilledVariables() override;
				void PrettyPrint(std::ostringstream&) const override;
				bool Resolve(DiagnosticContext& ctx) override;

				void Accept(SSAStatementVisitor& visitor) override;

				const SSAType GetType() const override;

			private:
				SSAType type_;
			};
		}
	}
}
