
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"

using namespace gensim::genc::ssa;

static bool TryThreadJump(SSAControlFlowStatement *ctrlflow)
{
	if(ctrlflow == nullptr) {
		return false;
	}

	bool changed = false;

	auto targets = ctrlflow->GetTargets();
	for(SSABlock *target : targets) {
		if(target->GetStatements().empty()) {
			continue;
		}

		SSAStatement *firststmt = target->GetStatements().front();
		if(SSAJumpStatement* jump = dynamic_cast<SSAJumpStatement*>(firststmt)) {
			bool replaced = ctrlflow->Replace(target, jump->Target());
			GASSERT(replaced == true);
			changed = true;
		}
	}
	return changed;
}

class JumpThreadingPass : public SSAPass
{
public:


	bool Run(SSAFormAction& action) const override
	{
		bool changed = false;
		for(auto block : action.Blocks) {
			changed |= TryThreadJump(block->GetControlFlow());
		}
		return changed;
	}

};


RegisterComponent0(SSAPass, JumpThreadingPass, "JumpThreading", "Perform jump threading");