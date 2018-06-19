/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/Enums.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSAValue.h"

#include <list>
#include <string>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAVariableReadStatement;
			class SSAVariableWriteStatement;
			class SSAVariableKillStatement;

			class SSASymbol : public SSAValue
			{
			public:
				SymbolType SType;

				std::list<SSAVariableReadStatement *> &Uses;

				std::string GetPrettyName() const
				{
					return pretty_name_;
				}

				std::string GetName() const override;

				void Unlink() override;

				bool IsFullyFixed() const;

				SSASymbol(SSAContext& context, std::string prettyname, const SSAType& type, SymbolType stype, SSASymbol *referenceTo = NULL);

				bool IsReference() const
				{
					return ReferenceTo != NULL;
				}

				const SSASymbol *GetReferencee() const
				{
					assert(IsReference());
					return ReferenceTo;
				}

				SSASymbol *GetReferencee()
				{
					assert(IsReference());
					return ReferenceTo;
				}

				void SetReferencee(SSASymbol *s)
				{
					assert(GetType().Reference);
					if(ReferenceTo != nullptr) {
						ReferenceTo->RemoveUse(this);
					}
					ReferenceTo = s;
					if(ReferenceTo != nullptr) {
						ReferenceTo->AddUse(this);
					}
				}

				const SSASymbol *ResolveReferences() const
				{
					if (IsReference()) return GetReferencee()->ResolveReferences();
					return this;
				}

				const std::list<SSAVariableKillStatement *> &GetDefs() const
				{
					return _defs;
				}

				const std::list<SSAVariableReadStatement *> &GetUses() const
				{
					return _uses;
				}

				const SSAType GetType() const override
				{
					return Type;
				}

				std::string ToString() const override;

				void SetName(const std::string &newname);

			private:
				const std::string pretty_name_;
				std::string name_;

				SSASymbol *ReferenceTo;
				SSAType Type;

				std::list<SSAVariableReadStatement *> _uses;
				std::list<SSAVariableKillStatement *> _defs;

				SSASymbol() = delete;
				SSASymbol(const SSASymbol&) = delete;
				SSASymbol(SSASymbol&&) = delete;
			};
		}
	}
}
