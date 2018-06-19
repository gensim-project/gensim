/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <map>
#include <vector>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAFormAction;
			class SSASymbol;
			class SSAVariableKillStatement;
			class SSAVariableReadStatement;

			namespace analysis
			{
				class VariableDefInfo
				{
				public:
					typedef std::vector<SSAVariableReadStatement*> uses_list_t;
					SSAVariableKillStatement *Statement()
					{
						return statement_;
					}

					uses_list_t &DominatedUses()
					{
						return dominated_uses_;
					}
					const uses_list_t &DominatedUses() const
					{
						return dominated_uses_;
					}

					uses_list_t &Uses()
					{
						return uses_;
					}
					const uses_list_t &Uses() const
					{
						return uses_;
					}

				private:
					SSAVariableKillStatement *statement_;
					uses_list_t dominated_uses_;
					uses_list_t uses_;
				};

				class VariableUseInfo
				{
				public:
					VariableUseInfo(SSAVariableReadStatement *stmt) : statement_(stmt) {}

					typedef std::vector<SSAVariableKillStatement*> defs_list_t;
					SSAVariableReadStatement *Statement()
					{
						return statement_;
					}

					defs_list_t &DominatingDefs()
					{
						return dominating_defs_;
					}
					const defs_list_t &DominatingDefs() const
					{
						return dominating_defs_;
					}

					defs_list_t &Defs()
					{
						return defs_;
					}
					const defs_list_t &Defs() const
					{
						return defs_;
					}

				private:
					SSAVariableReadStatement *statement_;
					defs_list_t dominating_defs_;
					defs_list_t defs_;
				};

				class VariableUseDefInfo
				{
				public:
					typedef std::vector<VariableDefInfo> def_list_t;
					typedef std::vector<VariableUseInfo> use_list_t;

					def_list_t &Defs()
					{
						return defs_;
					}
					const def_list_t &Defs() const
					{
						return defs_;
					}
					use_list_t &Uses()
					{
						return uses_;
					}
					const use_list_t &Uses() const
					{
						return uses_;
					}
				private:
					def_list_t defs_;
					use_list_t uses_;
				};

				class ActionUseDefInfo
				{
				public:
					const VariableUseDefInfo &Get(const SSASymbol *sym) const
					{
						return info_.at(sym);
					}
					void Set(const SSASymbol *sym, const VariableUseDefInfo &info)
					{
						info_[sym] = info;
					}

				private:
					std::map<const SSASymbol *, VariableUseDefInfo> info_;
				};

				class VariableUseDefAnalysis
				{
				public:
					ActionUseDefInfo Run(SSAFormAction *action);
					VariableUseDefInfo Run(SSAFormAction *action, SSASymbol *symbol);

				private:
					VariableUseInfo HandleRead(SSAVariableReadStatement *read);
					VariableDefInfo HandleKill(SSAVariableKillStatement *kill);
				};
			}
		}
	}
}

