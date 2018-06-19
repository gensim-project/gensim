/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/Parser.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRScope.h"

namespace gensim
{
	namespace genc
	{

/////////////////////////////////////////////////////////////
//                                                         //
// IRSymbol                                                //
//                                                         //
/////////////////////////////////////////////////////////////

		std::string IRSymbol::GetName() const
		{
			return "_Q" + Scope->GetName() + name;
		}

		std::string IRSymbol::GetLocalName() const
		{
			return name;
		}

/////////////////////////////////////////////////////////////
//                                                         //
// IRScope                                                 //
//                                                         //
/////////////////////////////////////////////////////////////

		uint64_t IRScope::_global_scope_id = 0;

		const IRSymbol *IRScope::GetSymbol(const std::string Name) const
		{
			assert(ContainingScope != this);
			assert(!IRSymbol::IsQualifiedName(Name));
			if (localScope.find(Name) == localScope.end()) {
				if (ContainingScope != NULL)
					return ContainingScope->GetSymbol(Name);
				else
					return NULL;
			}
			return localScope.at(Name);
		}

		bool IRScope::GetSymbolType(const std::string Name, IRType &out) const
		{
			const IRSymbol *sym = GetSymbol(Name);
			if (sym) {
				out = sym->Type;
				return true;
			}
			return false;
		}

		bool IRScope::IsConstant(const std::string str) const
		{
			return GetSymbol(str) && (GetSymbol(str)->SType == Symbol_Constant);
		}

		const IRAction &IRScope::GetContainingAction() const
		{
			assert(containingAction);
			return *containingAction;
		}

		IRSymbol *IRScope::InsertSymbol(const std::string Name, const IRType &Type, SymbolType stype)
		{
			assert(!IRSymbol::IsQualifiedName(Name));
			IRSymbol *sym = new IRSymbol(Name, Type, stype, this);
			return (localScope[sym->GetLocalName()] = sym);
		}

		const IRScope::SymbolTable IRScope::GetAllSymbols() const
		{
			SymbolTable symbols = GetLocalSymbols();

			if (ContainingScope != NULL) {
				const SymbolTable &parentSymbols = ContainingScope->GetAllSymbols();
				symbols.insert(parentSymbols.begin(), parentSymbols.end());
			}

			return symbols;
		}

		IRScope &IRScope::CreateFunctionScope(const IRAction &Action)
		{
			IRScope *Scope = new IRScope(Action, Action.Context);
			Scope->ContainingScope = &Action.Context.GlobalScope;
			Scope->Type = SCOPE_FUNCTION;

			return *Scope;
		}

		IRScope &IRScope::CreateGlobalScope(const GenCContext &context)
		{
			IRScope *Scope = new IRScope(context);
			Scope->ContainingScope = NULL;
			Scope->Type = SCOPE_GLOBAL;

			return *Scope;
		}
	}
}
