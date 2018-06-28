/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/SSAActionPrototype.h"
#include "genC/ssa/SSAValue.h"

#include "DiagnosticContext.h"

#include <list>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <functional>

namespace gensim
{
	namespace arch
	{
		class ArchDescription;
	}
	namespace isa
	{
		class ISADescription;
	}

	namespace genc
	{
		class IRAction;
		class GenCContext;

		namespace ssa
		{
			class SSAStatement;
			class SSAVariableReadStatement;
			class SSAVariableKillStatement;
			class SSABlock;
			class SSASymbol;

			class SSAActionBase : public SSAValue
			{
			public:
				SSAActionBase(SSAContext& context, const SSAActionPrototype& prototype);
				virtual ~SSAActionBase();

				const SSAActionPrototype& GetPrototype() const
				{
					return prototype_;
				}

				const SSAType GetType() const override
				{
					return _type;
				}

				bool HasAttribute(ActionAttribute::EActionAttribute attribute) const
				{
					return prototype_.HasAttribute(attribute);
				}

				virtual void Destroy();
			private:
				SSAActionPrototype prototype_;
				const SSAType _type;
			};

			class SSAExternalAction : public SSAActionBase
			{
			public:
				SSAExternalAction(SSAContext& context, const SSAActionPrototype &prototype) : SSAActionBase(context, prototype)
				{
					assert(prototype.HasAttribute(gensim::genc::ActionAttribute::External));
				}
				virtual ~SSAExternalAction() { }

				void Unlink() override { }
				void Destroy() override
				{
					Dispose();
				}
			};

			/**
			 * Class representing an IRAction converted into SSA form.
			 */
			class SSAFormAction : public SSAActionBase
			{
			public:
				typedef std::set<SSASymbol *> SymbolTableType;
				typedef SymbolTableType::const_iterator SymbolTableConstIterator;
				typedef SymbolTableType::iterator SymbolTableIterator;

				typedef std::list<SSABlock *> BlockList;
				typedef BlockList::const_iterator BlockListConstIterator;
				typedef BlockList::iterator BlockListIterator;

				typedef std::vector<SSAStatement *> StatementList;

				/**
				 * Constructs a new SSAFormAction object.
				 * @param prototype Associated SSA Action Prototype.
				 */
				SSAFormAction(SSAContext& context, const SSAActionPrototype &prototype);

				StatementList GetStatements(std::function<bool(SSAStatement*)>) const;

				std::list<SSABlock *> Blocks;
				bool ContainsBlock(const SSABlock *block) const;
				void AddBlock(SSABlock *block);
				void RemoveBlock(SSABlock *block);

				SSABlock *EntryBlock;

				void AddSymbol(SSASymbol *new_symbol);
				const SymbolTableType &Symbols() const
				{
					return _symbols;
				}
				void RemoveSymbol(SSASymbol *symbol);
				const SSASymbol *GetSymbol(const std::string symname) const;
				SSASymbol *GetSymbol(const std::string symname);
				const SSASymbol *GetSymbol(const char *symname) const;
				SSASymbol *GetSymbol(const char *symname);

				virtual std::string GetName() const override;

				std::vector<SSASymbol *> ParamSymbols;

				SSAFormAction *Clone() const;

				const arch::ArchDescription *Arch;
				const isa::ISADescription *Isa;

				const IRAction *GetAction()
				{
					return action_;
				}

				const IRAction *GetAction() const
				{
					return action_;
				}

				void LinkAction(const IRAction *action);

				bool Resolve(DiagnosticContext &ctx);

				bool DoFixednessAnalysis();

				void Unlink() override;
				void Destroy() override;

				/**
				 * Returns a list of reads dominated by the given store
				 */
				std::list<SSAVariableReadStatement *> GetDominatedReads(const SSAVariableKillStatement *stmt) const;

				/**
				 * Returns true if the given variable write might dominate a dynamic read
				 */
				bool HasDynamicDominatedReads(const SSAVariableKillStatement *stmt) const;

				/* --- Optimisation passes, return true if a change was made --- */

				// Check that all paths through this action have a return statement
				bool DoCheckReturn() const;

				std::string ToString() const override;

			private:
				SymbolTableType _symbols;
				const IRAction *action_;
			};
		}
	}
}
