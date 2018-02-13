#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAStatements.h"

using namespace gensim::genc::ssa;

class DeadWriteEliminationPass : public SSAPass
{
public:
	virtual ~DeadWriteEliminationPass()
	{

	}

	bool CheckSymbol(SSASymbol *sym) const
	{
		// if the symbol is a reference parameter, then writes cannot be eliminated
		if(sym->GetType().Reference) {
			return false;
		}

		for(auto use : ((SSAValue*)sym)->GetUses()) {
			// if the use is anything other than a variable write or ssaformaction, then writes to the symbol cannot be eliminated
			if(dynamic_cast<SSAVariableWriteStatement*>(use)) {
				continue;
			}
			if(dynamic_cast<SSAFormAction*>(use)) {
				continue;
			}
			return false;
		}
		return true;
	}

	bool RemoveWrites(SSASymbol *sym) const
	{
		std::set<SSAVariableWriteStatement*> writes;
		for(auto use : ((SSAValue*)sym)->GetUses()) {
			if(dynamic_cast<SSAVariableWriteStatement*>(use)) {
				SSAVariableWriteStatement *write = (SSAVariableWriteStatement*)use;
				writes.insert(write);
			}
		}
		for(auto write : writes) {
			write->Parent->RemoveStatement(*write);
			write->Dispose();
			delete write;
		}
		return !writes.empty();
	}

	bool Run(SSAFormAction& action) const override
	{
		bool changed = false;

		std::set<SSASymbol*> dead_syms;
		for (auto sym : action.Symbols()) {
			if(CheckSymbol(sym)) {
				dead_syms.insert(sym);
			}
		}
		for(auto sym : dead_syms) {
			changed |= RemoveWrites(sym);
		}

		return changed;
	}
};

RegisterComponent0(SSAPass, DeadWriteEliminationPass, "DeadWriteElimination", "Remove any writes to variables that do not dominate reads")
