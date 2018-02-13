/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSACastStatement.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAIfStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/statement/SSASwitchStatement.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/analysis/ReachabilityAnalysis.h"

using namespace gensim::genc::ssa;


class UnreachableBlockEliminationPass : public SSAPass
{

	bool RemoveBlock(SSABlock *block) const
	{
		GASSERT(block->GetPredecessors().empty());
		while(!block->GetStatements().empty()) {

			auto statement = block->GetStatements().back();
			block->RemoveStatement(*statement);

			statement->Dispose();
			delete statement;
		}

		block->Parent->RemoveBlock(block);

		block->Dispose();
		delete block;

		return true;
	}

	std::set<SSABlock*> FindUnreachableBlocks(SSAFormAction &action) const
	{
		std::set<SSABlock*> reachability_sources = {action.EntryBlock};


		// Mark blocks which have statements which are operands of phi nodes as
		// reachability sources. This stops us from deleting blocks which have
		// indirect references in the form of phi nodes.
		for(auto block : action.Blocks) {
			for(auto stmt : block->GetStatements()) {
				if(SSAPhiStatement *phi = dynamic_cast<SSAPhiStatement*>(stmt)) {
					for(auto op : phi->Get()) {
						((SSAStatement*)op)->Parent->CheckDisposal();
						reachability_sources.insert(((SSAStatement*)op)->Parent);
					}
				}
			}
		}

		ReachabilityAnalysis ra;
		auto reachable_blocks = ra.GetReachable(action, reachability_sources);

		// invert reachability set
		std::set<SSABlock *> all_blocks;
		std::set<SSABlock *> unreachable_blocks;
		all_blocks.insert(action.Blocks.begin(), action.Blocks.end());
		std::set_difference(all_blocks.begin(), all_blocks.end(), reachable_blocks.begin(), reachable_blocks.end(), std::inserter(unreachable_blocks, unreachable_blocks.begin()));
		return unreachable_blocks;
	}

	void RemoveBlocks(const std::set<SSABlock*>& blocks) const
	{
		// Firstly, remove all operands from phi nodes found in each block.
		for(auto block : blocks) {
			for(auto stmt : block->GetStatements()) {
				if(SSAPhiStatement *phi = dynamic_cast<SSAPhiStatement*>(stmt)) {
					phi->Set({});
				}
			}
		}

		// Secondly, remove all of the other statements from each block so that we can
		// delete the unreachable blocks in any order (otherwise we have to
		// worry about the blocks having references to each other)
		for(auto& block : blocks) {

			for(auto rit = block->GetStatements().rbegin(); rit != block->GetStatements().rend(); ++rit) {
				block->RemoveStatement(**rit);
				(*rit)->Dispose();
				delete *rit;
			}

		}

		for(auto& block : blocks) {
			RemoveBlock(block);
		}
	}

	bool RemoveUnreachableBlocks(SSAFormAction &action) const
	{
		auto unreachable_blocks = FindUnreachableBlocks(action);
		RemoveBlocks(unreachable_blocks);

		return !unreachable_blocks.empty();
	}

	bool Run(SSAFormAction& action) const override
	{
		return RemoveUnreachableBlocks(action);
	}

};

RegisterComponent0(SSAPass, UnreachableBlockEliminationPass, "UnreachableBlockElimination", "Remove any blocks which are not reachable")
