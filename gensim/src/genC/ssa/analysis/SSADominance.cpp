/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/analysis/SSADominance.h"
#include "genC/ssa/analysis/ControlFlowGraphAnalyses.h"

#include <algorithm>
#include <iterator>
#include <unordered_set>

#include <wutils/vbitset.h>

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::analysis;

SSADominance::dominance_info_t SSADominance::Calculate(const SSAFormAction* action)
{
	std::map<const SSABlock*, wutils::vbitset> doms;
	doms.emplace(action->EntryBlock, wutils::vbitset(action->GetBlocks().size()));
	doms.at(action->EntryBlock).set(action->EntryBlock->GetID(), 1);

	for(const auto block : action->GetBlocks()) {
		if(block == action->EntryBlock) {
			continue;
		}
		wutils::vbitset the_set(action->GetBlocks().size());
		the_set.set_all();
		doms.emplace(block, the_set);
	}

	PredecessorAnalysis predecessors (action);

	bool changes = true;
	while(changes) {
		changes = false;

		for(auto block : action->GetBlocks()) {
			if(block == action->EntryBlock) {
				continue;
			}

			const auto &old_dom = doms.at(block);

			wutils::vbitset new_dom(action->GetBlocks().size());
			for(auto pred : predecessors.GetPredecessors(block)) {
				auto &pred_dom = doms.at(pred);
				new_dom &= pred_dom;
			}
			new_dom.set(block->GetID(), 1);

			if(new_dom != old_dom) {
				doms.at(block) = new_dom;
				changes = true;
			}
		}

	}
	
	dominance_info_t dominance;
	
	for(auto block : action->GetBlocks()) {
		
		for(auto dominator : action->GetBlocks()) {
			if(doms.at(block).get(dominator->GetID())) {
				dominance[block].insert(dominator);
			}
		}
	}
	
	return dominance;
}

std::string SSADominance::Print(const dominance_info_t& dominance) const
{
	std::ostringstream str;
	for(auto block : dominance) {
		str << "Block " << block.first->GetName() << " dominated by:" << std::endl;
		for(auto dominated : block.second) {
			str << " - " << dominated->GetName() << std::endl;
		}
	}
	return str.str();
}

