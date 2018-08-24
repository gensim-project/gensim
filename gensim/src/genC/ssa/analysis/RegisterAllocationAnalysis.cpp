/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/analysis/RegisterAllocationAnalysis.h"
#include "genC/ssa/analysis/ControlFlowGraphAnalyses.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"

#include <list>
#include <set>

using namespace gensim::genc::ssa;

class LiveRange
{
public:
	SSAStatement *Start, *End;

	bool IntersectsWith(const LiveRange *other) const
	{
		if (other == this) return true;
		return !((Start->GetIndex() >= other->End->GetIndex()) || (End->GetIndex() <= other->Start->GetIndex()));
	}
};

const RegisterAllocation RegisterAllocationAnalysis::Allocate(const SSAFormAction *action, int max_regs)
{
	RegisterAllocation allocation;

	PredecessorAnalysis preds(action);
	std::list<SSABlock *> work_list;
	std::set<SSABlock *> seen_list;

	const auto& exit_blocks = GetExitBlocks(*action);
	for (const auto& block : exit_blocks) {
		work_list.push_back(block);
	}

	while (!work_list.empty()) {
		auto block = work_list.front();
		work_list.pop_front();

		if (seen_list.count(block)) continue;
		seen_list.insert(block);

		ProcessBlock(allocation, block);

		for (const auto& pred : preds.GetPredecessors(block)) {
			work_list.push_back(pred);
		}
	}

	return allocation;
}

const std::set<SSABlock *> RegisterAllocationAnalysis::GetExitBlocks(const SSAFormAction& action) const
{
	SuccessorAnalysis succs(&action);

	std::set<SSABlock *> exit_blocks;
	for (const auto& block : action.GetBlocks()) {
		if (succs.GetSuccessors(block).size() == 0) {
			exit_blocks.insert(block);
		}
	}

	return exit_blocks;
}

void RegisterAllocationAnalysis::ProcessBlock(RegisterAllocation& allocation, SSABlock* block) const

{
	auto& statements = block->GetStatements();

	std::map<SSAStatement *, LiveRange *> live_ranges;
	for (auto SI = statements.rbegin(), SE = statements.rend(); SI != SE; ++SI) {
		SSAStatement *stmt = *SI;
		for (auto use : stmt->GetUsedStatements()) {
			if (!live_ranges.count(use)) {
				auto range = new LiveRange();
				range->Start = use;
				range->End = stmt;
				live_ranges[use] = range;
			}
		}
	}

	//fprintf(stderr, "--- BLOCK %s\nLIVE RANGES:\n", block->GetName().c_str());
	for (const auto& r : live_ranges) {
		uint64_t registers = 0xffff;

		//fprintf(stderr, "  RANGE %s -- %s\n", r.second->Start->GetName().c_str(), r.second->End->GetName().c_str());
		for (const auto& i : live_ranges) {
			if (i.first == r.first) continue;

			if (r.second->IntersectsWith(i.second)) {
				//fprintf(stderr, "    ISECT %s -- %s\n", i.second->Start->GetName().c_str(), i.second->End->GetName().c_str());
				if (allocation.IsStatementAllocated(i.first)) {
					registers &= ~(1 << allocation.GetRegisterForStatement(i.first));
				}
			}
		}

		for (int i = 0; i < 16; i++) {
			if (registers & (1 << i)) {
				allocation.AllocateStatement(r.first, i);
				break;
			}
		}
	}
}
