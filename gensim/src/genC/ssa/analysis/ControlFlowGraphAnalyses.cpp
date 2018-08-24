/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/analysis/ControlFlowGraphAnalyses.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"

using namespace gensim::genc::ssa;

SuccessorAnalysis::SuccessorAnalysis(const SSAFormAction* action)
{
	BuildSuccessors(action);
}

void SuccessorAnalysis::BuildSuccessors(const SSAFormAction* action)
{
	for (const auto& block : action->GetBlocks()) {
		successors_[block] = block->GetSuccessors();
	}
}

PredecessorAnalysis::PredecessorAnalysis(const SSAFormAction* action)
{
	BuildPredecessors(action);
}

void PredecessorAnalysis::BuildPredecessors(const SSAFormAction* action)
{
	SuccessorAnalysis successors (action);

	for(auto block : action->GetBlocks()) {
		predecessors_[block].resize(0);
	}

	for(auto block : action->GetBlocks()) {
		for(auto successor : successors.GetSuccessors(block)) {
			predecessors_[successor].push_back(block);
		}
	}
}
