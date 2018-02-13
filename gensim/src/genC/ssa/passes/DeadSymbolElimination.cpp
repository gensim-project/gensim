/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/passes/SSAPass.h"

#include "genC/ssa/statement/SSAControlFlowStatement.h"

#include <list>

using namespace gensim::genc::ssa;

class DeadSymbolElimination  : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		std::set<SSASymbol*> dead_syms;
		for(SSASymbol *sym : action.Symbols()) {
			if(((SSAValue*)sym)->GetUses().size() == 1) {
				bool is_param = false;
				for(auto param : action.ParamSymbols) {
					if(sym == param) {
						is_param = true;
						break;
					}
				}
				if(!is_param) {
					GASSERT(((SSAValue*)sym)->GetUses().front() == &action);
					dead_syms.insert((SSASymbol*)sym);
				}
			}
		}

		for(auto sym : dead_syms) {
			action.RemoveSymbol(sym);
			sym->Dispose();
			delete sym;
		}

		return !dead_syms.empty();
	}
};

RegisterComponent0(SSAPass, DeadSymbolElimination, "DeadSymbolElimination", "Eliminate unused symbols from the symbol table");