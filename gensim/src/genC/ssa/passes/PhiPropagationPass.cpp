/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/* Pass to propagate values between phi nodes. If a phi node refers to another
 * phi node, replace that reference with references to each member of the other
 * phi node */

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSAControlFlowStatement.h"
#include "genC/ssa/analysis/VariableUseDefAnalysis.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "genC/ssa/analysis/SSADominance.h"

using namespace gensim::genc::ssa;

class PhiPropagationPass : public SSAPass
{
public:
	bool ProcessPhi(SSAPhiStatement* phi)  const
	{
		// this phi node has a single member (which is another phi node) so
		// propagate the values of the 'another' node to this node
		SSAPhiStatement *another_node = (SSAPhiStatement*)phi->Get().front();
		phi->Set(another_node->Get());
		return true;
	}

	bool Run(SSAFormAction& action) const  override
	{
		bool changed = false;
		for(auto block : action.Blocks) {
			for(auto stmt : block->GetStatements()) {
				if(SSAPhiStatement *phi = dynamic_cast<SSAPhiStatement*>(stmt)) {
					if(phi->Get().size() == 1 && dynamic_cast<SSAPhiStatement*>(phi->Get().front())) {
						changed |= ProcessPhi(phi);
					}
				}
			}
		}

		return changed;
	}
};

RegisterComponent0(SSAPass, PhiPropagationPass, "PhiPropagation", "Perform phi propagation");
