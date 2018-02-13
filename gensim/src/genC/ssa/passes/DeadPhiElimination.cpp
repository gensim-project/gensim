/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/analysis/ReachabilityAnalysis.h"
#include "genC/ssa/statement/SSAPhiStatement.h"

using namespace gensim::genc::ssa;

// This pass should identify blocks which are unreachable by control flow,
// and remove their statements from the operand lists of any phi nodes which
// reference them.

static bool ProcessPhi(SSAPhiStatement *phi, std::set<SSABlock*> reachable_blocks)
{
	auto members = phi->Get();
	std::set<SSAStatement *> pre_check;
	for(auto i : members) {
		pre_check.insert((SSAStatement*)i);
	}
	std::set<SSAStatement *> checked;

	for(auto i : pre_check) {
		if(reachable_blocks.count(i->Parent)) {
			checked.insert(i);
		}
	}

	SSAStatement::operand_list_t ops;
	for(auto i : checked) {
		ops.push_back(i);
	}
	phi->Set(ops);

	return checked != pre_check;
}

class DeadPhiElimination : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		ReachabilityAnalysis ra;
		auto reachable_blocks = ra.GetReachable(action);

		bool changed = false;

		for(auto block : action.Blocks) {
			for(auto stmt : block->GetStatements()) {
				if(SSAPhiStatement *phi = dynamic_cast<SSAPhiStatement*>(stmt)) {
					changed |= ProcessPhi(phi, reachable_blocks);
				}
			}
		}
		return false;
	}

};

RegisterComponent0(SSAPass, DeadPhiElimination, "DeadPhiElimination", "Remove phi operands which are in unreachable blocks")
