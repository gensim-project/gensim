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
				using VariableDefInfo = SSAVariableKillStatement*;
				using VariableUseInfo = SSAVariableReadStatement*;

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
					VariableUseDefInfo &Get(const SSASymbol *sym)
					{
						return info_[sym];
					}

				private:
					std::map<const SSASymbol *, VariableUseDefInfo> info_;
				};

				class VariableUseDefAnalysis
				{
				public:
					ActionUseDefInfo Run(SSAFormAction *action);

				};
			}
		}
	}
}

