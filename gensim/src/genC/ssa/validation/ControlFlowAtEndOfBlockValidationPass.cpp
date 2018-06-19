/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/validation/SSAActionValidationPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/SSAType.h"
#include "DiagnosticContext.h"
#include "ComponentManager.h"
#include "genC/ssa/analysis/ReachabilityAnalysis.h"

using gensim::DiagnosticContext;
using gensim::DiagnosticClass;

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

// This validation pass asserts that all REACHABLE blocks have a control flow
// statement

class ControlFlowAtEndOfBlockValidationPass : public SSAActionValidationPass
{
	bool Run(const SSAFormAction* action, DiagnosticContext& ctx) override
	{
		bool success = true;
		ReachabilityAnalysis ra;
		auto reachable = ra.GetReachable(*action);

		for(auto block : action->Blocks) {
			if(!reachable.count(block)) {
				continue;
			}
			if(block->GetStatements().empty()) {
				ctx.AddEntry(DiagnosticClass::Error, "Block has no statements", block->GetDiag());
				success = false;
			} else if(block->GetControlFlow() == nullptr) {
				ctx.AddEntry(DiagnosticClass::Error, "Block does not have control flow", block->GetDiag());
				success = false;
			}
		}

		return success;
	}
};

RegisterComponent0(SSAActionValidationPass, ControlFlowAtEndOfBlockValidationPass, "ControlFlowAtEndOfBlock", "Check that all blocks end with a control flow instruction");
