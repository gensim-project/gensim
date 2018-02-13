/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/analysis/CallGraphAnalysis.h"
#include "genC/ssa/statement/SSACallStatement.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::ssa;

CallGraph CallGraphAnalysis::Analyse(const SSAContext& ctx) const
{
	CallGraph graph;

	for(auto action : ctx.Actions()) {
		SSAFormAction *faction = dynamic_cast<SSAFormAction*>(action.second);
		if (faction == nullptr) continue;
		
		auto callees = GetCallees(faction);
		for(auto callee : callees) {
			graph.AddCallee(faction, callee);
		}
	}
	return graph;
}

CallGraph::callee_set_t CallGraphAnalysis::GetCallees(SSAFormAction* action) const
{
	CallGraph::callee_set_t callees;
	for(auto block : action->Blocks) {
		for(auto stmt : block->GetStatements()) {
			SSACallStatement *call = dynamic_cast<SSACallStatement*>(stmt);
			if(call != nullptr && dynamic_cast<SSAFormAction *>(call->Target()) != nullptr) {
				callees.insert(call->Target());
			}
		}
	}
	return callees;
}
