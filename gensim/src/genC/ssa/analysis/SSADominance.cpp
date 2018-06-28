/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/analysis/SSADominance.h"
#include "genC/ssa/analysis/ControlFlowGraphAnalyses.h"

#include <algorithm>
#include <iterator>
#include <unordered_set>

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::analysis;

SSADominance::dominance_info_t SSADominance::Calculate(const SSAFormAction* action)
{
	dominance_info_t dominance;

	dominance[action->EntryBlock].insert(action->EntryBlock);

	for(const auto block : action->Blocks) {
		if(block == action->EntryBlock) {
			continue;
		}
		dominance[block].insert(action->Blocks.begin(), action->Blocks.end());
	}

	PredecessorAnalysis predecessors (action);

	bool changes = true;
	while(changes) {
		changes = false;

		for(auto block : action->Blocks) {
			if(block == action->EntryBlock) {
				continue;
			}

			const auto &old_dom = dominance.at(block);

			block_dominators_t new_dom;
			for(auto pred : predecessors.GetPredecessors(block)) {
				auto &pred_dom = dominance.at(pred);
				std::set_intersection(old_dom.begin(), old_dom.end(), pred_dom.begin(), pred_dom.end(), std::inserter(new_dom, new_dom.begin()));
			}
			new_dom.insert(block);

			if(new_dom != old_dom) {
				dominance[block] = new_dom;
				changes = true;
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

