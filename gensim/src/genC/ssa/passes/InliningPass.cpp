/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAInliner.h"
#include "genC/ssa/passes/SSAPass.h"

#include "genC/ssa/statement/SSACallStatement.h"
#include "genC/ssa/statement/SSAControlFlowStatement.h"

#include <list>

using namespace gensim::genc::ssa;

static bool InlineOneCall(SSAFormAction &action)
{
	SSAInliner inliner;
	for(SSABlock *block : action.Blocks) {
		for(SSAStatement *statement: block->GetStatements()) {
			SSACallStatement *call = dynamic_cast<SSACallStatement*>(statement);

			if(call != nullptr) {
				// We can only inline functions that have targets which are SSAFormActions
				if (dynamic_cast<SSAFormAction *>(call->Target()) == nullptr) {
					continue;
				}

				// Skip inlining functions which are marked as noinline
				if(call->Target()->HasAttribute(gensim::genc::ActionAttribute::NoInline)) {
					continue;
				}

				inliner.Inline(call);
				return true;
			}
		}
	}
	return false;
}

class InliningPass : public SSAPass
{
public:
	virtual ~InliningPass()
	{

	}

	bool Run(SSAFormAction& action) const override
	{
		bool changed = false;
		bool anychanged = false;

		do {
			changed = InlineOneCall(action);
			anychanged |= changed;
		} while(changed);

		return anychanged;
	}

};

RegisterComponent0(SSAPass, InliningPass, "Inlining", "Inline all calls")
