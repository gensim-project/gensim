/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   IRScope.h
 * Author: s0803652
 *
 * Created on September 24, 2012, 3:04 PM
 */

#ifndef IRSCOPE_H
#define IRSCOPE_H

#include "genC/Enums.h"
#include "IRAction.h"

#include <map>

namespace gensim
{
	namespace genc
	{

		class IRAction;

		class IRScope;

		class GenCContext;

		class IRSymbol
		{
		public:
			IRType Type;
			IRScope *Scope;
			SymbolType SType;

			IRSymbol(std::string _name, IRType _type, SymbolType _stype, IRScope *_scope) : Type(_type), Scope(_scope), SType(_stype), name(_name) {}

			std::string GetName() const;
			std::string GetLocalName() const;

			static bool IsQualifiedName(const std::string n)
			{
				return n[0] == '_' && n[1] == 'Q';
			}

		private:
			std::string name;
		};

		class IRScope
		{
		public:
			typedef std::map<std::string, IRSymbol *> SymbolTable;
			typedef SymbolTable::iterator SymbolTableIterator;
			typedef SymbolTable::const_iterator SymbolTableConstIterator;

			static IRScope &CreateFunctionScope(const IRAction &Action);
			static IRScope &CreateGlobalScope(const GenCContext &context);

			enum ScopeType {
				SCOPE_GLOBAL,
				SCOPE_FUNCTION,
				SCOPE_BODY,
				SCOPE_LOOP,
				SCOPE_CASE,
				SCOPE_SWITCH
			};

			explicit IRScope(IRScope &_ContainingScope, ScopeType _type) : containingAction(_ContainingScope.containingAction), containingContext(_ContainingScope.containingContext), ContainingScope(&_ContainingScope), Type(_type), _scope_id(_global_scope_id++)
			{
				assert(ContainingScope != this);
			};

			bool GetSymbolType(std::string Name, IRType &out) const;
			const IRSymbol *GetSymbol(std::string Name) const;
			IRSymbol *InsertSymbol(std::string Name, const IRType &Type, SymbolType stype);

			std::string GetName() const
			{
				std::stringstream str;
				str << "scope_" << _scope_id;
				return str.str();
			}  // at some point this should produce something more meaningful

			const SymbolTable &GetLocalSymbols() const
			{
				return localScope;
			}
			const SymbolTable GetAllSymbols() const;

			inline bool ScopeBreakable() const
			{
				bool breakable = ((Type == SCOPE_CASE) || (Type == SCOPE_LOOP));
				if (breakable) return true;
				if (ContainingScope != NULL) return ContainingScope->ScopeBreakable();
				return false;
			}

			inline bool ScopeContinuable() const
			{
				bool breakable = ((Type == SCOPE_LOOP));
				if (breakable) return true;
				if (ContainingScope != NULL) return ContainingScope->ScopeContinuable();
				return false;
			}

			inline const IRScope *GetContainingLoop() const
			{
				if (Type == SCOPE_LOOP) return this;
				if (ContainingScope == NULL) return NULL;
				return ContainingScope->GetContainingLoop();
			}

			IRScope *ContainingScope;

			ScopeType Type;

			bool IsConstant(std::string str) const;
			const IRAction &GetContainingAction() const;

		private:
			static uint64_t _global_scope_id;
			uint64_t _scope_id;

			const IRAction *containingAction;
			const GenCContext &containingContext;

			std::map<std::string, IRSymbol *> localScope;

			IRScope();

			explicit IRScope(const IRAction &_action, const GenCContext &_context) : containingAction(&_action), containingContext(_context), ContainingScope(NULL), _scope_id(_global_scope_id++) {}
			explicit IRScope(const GenCContext &context) : containingAction(NULL), containingContext(context), ContainingScope(NULL), _scope_id(_global_scope_id++) {}
			explicit IRScope(const IRScope &) = delete;
		};
	}
}

#endif /* IRSCOPE_H */
