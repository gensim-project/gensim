/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/analysis/SSADominance.h"
#include "genC/ssa/analysis/DominanceFrontierAnalysis.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::analysis;

// Returns true if x strictly dominates y
static bool StrictlyDominates(SSADominance::dominance_info_t &dominance, SSABlock *x, SSABlock *y)
{
	if(x == y) {
		return false;
	}
	return dominance.at(y).count(x);
}

// Return true if x dominates any predecessor of y
static bool DominatesAPredecessor(SSADominance::dominance_info_t &dominance, SSABlock *x, SSABlock *y)
{
	for(auto pred : y->GetPredecessors()) {
		if(dominance.at(y).count(x)) {
			return true;
		}
	}
	return false;
}

DominanceFrontierAnalysis::dominance_frontier_t DominanceFrontierAnalysis::GetDominanceFrontier(SSABlock* n) const
{
	SSADominance da;
	auto dominance = da.Calculate(n->Parent);
	dominance_frontier_t frontier;

	// just do it super dumb
	SSAFormAction *action = n->Parent;

	for(auto x : action->Blocks) {
		if(!StrictlyDominates(dominance, n, x) && DominatesAPredecessor(dominance, n, x)) {
			frontier.insert(x);
		}
	}

	return frontier;
}
