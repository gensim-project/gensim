#include "define.h"

#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/analysis/VariableUseDefAnalysis.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::analysis;

ActionUseDefInfo VariableUseDefAnalysis::Run(SSAFormAction* action)
{
	ActionUseDefInfo info;
	for(auto sym : action->Symbols()) {
		info.Set(sym, Run(action, sym));
	}

	return info;
}

VariableUseDefInfo VariableUseDefAnalysis::Run(SSAFormAction* action, SSASymbol* symbol)
{
	VariableUseDefInfo info;

	for(auto use : ((SSAValue*)symbol)->GetUses()) {
		if(SSAVariableReadStatement *read = dynamic_cast<SSAVariableReadStatement*>(use)) {
			info.Uses().push_back(HandleRead(read));
		} else if(SSAVariableKillStatement *kill = dynamic_cast<SSAVariableKillStatement*>(use)) {
			info.Defs().push_back(HandleKill(kill));
		}
	}

	return info;
}

VariableUseInfo VariableUseDefAnalysis::HandleRead(SSAVariableReadStatement* read)
{
	VariableUseInfo use_info (read);
	SSASymbol *symbol = read->Target();

	// check each other use of the variable to see if it is dominated etc.
	for(auto use : ((SSAValue*)symbol)->GetUses()) {

	}

	return use_info;
}

VariableDefInfo VariableUseDefAnalysis::HandleKill(SSAVariableKillStatement* kill)
{
	UNIMPLEMENTED;
}
