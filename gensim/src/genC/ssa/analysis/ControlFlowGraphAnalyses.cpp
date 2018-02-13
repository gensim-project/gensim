/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
	for (const auto& block : action->Blocks) {
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

	for(auto block : action->Blocks) {
		predecessors_[block].resize(0);
	}

	for(auto block : action->Blocks) {
		for(auto successor : successors.GetSuccessors(block)) {
			predecessors_[successor].push_back(block);
		}
	}
}
