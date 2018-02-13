/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/analysis/ReachabilityAnalysis.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"

#include <map>
#include <vector>

using namespace gensim::genc::ssa;

std::set<SSABlock*> ReachabilityAnalysis::GetReachable(const SSAFormAction& action)
{
	return GetReachable(action, {action.EntryBlock});
}

std::set<SSABlock*> ReachabilityAnalysis::GetReachable(const SSAFormAction& action, const std::set<SSABlock*>& reachability_sources)
{
	std::map<SSABlock*, bool> is_block_reachable;
	for(auto i : action.Blocks) {
		is_block_reachable[i] = reachability_sources.count(i);
	}

	std::vector<SSABlock *> work_list (reachability_sources.begin(), reachability_sources.end());
	std::set<SSABlock *> processed_blocks;

	while(!work_list.empty()) {
		auto block = work_list.back();
		work_list.pop_back();

		if(processed_blocks.count(block)) {
			continue;
		}
		processed_blocks.insert(block);

		is_block_reachable[block] = true;

		// Cannot use GetSuccessors straight away since we might be called from 
		// a context where the SSA is not valid (e.g., empty blocks)
		
		if(!block->GetStatements().empty()) {
			if(block->GetControlFlow() != nullptr) {
				for(auto succ : block->GetSuccessors()) {
					work_list.push_back(succ);
				}
			}
		}
	}

	std::set<SSABlock*> reachable;
	for(auto i : is_block_reachable) {
		if(i.second) {
			reachable.insert(i.first);
		}
	}
	return reachable;
}
