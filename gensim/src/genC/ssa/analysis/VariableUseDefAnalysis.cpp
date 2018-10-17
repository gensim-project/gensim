/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
	for(auto block : action->GetBlocks()) {
		for(auto stmt : block->GetStatements()) {

			if(auto read = dynamic_cast<SSAVariableReadStatement*>(stmt)) {
				info.Get(read->Target()).Uses().push_back(read);
			} else if(auto write = dynamic_cast<SSAVariableKillStatement*>(stmt)) {
				info.Get(write->Target()).Defs().push_back(write);
			}
		}
	}

	return info;
}
