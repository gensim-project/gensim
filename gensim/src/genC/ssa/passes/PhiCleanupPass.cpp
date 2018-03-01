/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include "define.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSAControlFlowStatement.h"
#include "genC/ssa/analysis/VariableUseDefAnalysis.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "genC/ssa/analysis/SSADominance.h"

#include "util/Tarjan.h"

using namespace gensim::genc::ssa;

class PhiCleanupPass : public SSAPass
{
public:
	// Get all of the phi statements in this block which refer only to previous
	// statements in the same block
	std::list<SSAPhiStatement*> GetDominatedPhiNodes(SSABlock *block) const
	{
		std::list<SSAPhiStatement*> nodes;
		for(auto stmt : block->GetStatements()) {
			if(SSAPhiStatement *phi = dynamic_cast<SSAPhiStatement*>(stmt)) {
				if(phi->Get().size() == 1) {
					SSAStatement *member = (SSAStatement*)phi->Get().front();
					if(member->Parent == block) {
						nodes.push_back(phi);
					}
				}
			}
		}
		return nodes;
	}

	void RemoveDominatedPhiNode(SSAPhiStatement *node) const
	{
		GASSERT(node->Get().size() == 1);
		GASSERT(((SSAStatement*)node->Get().front())->Parent == node->Parent);

		SSAStatement *solo_member = (SSAStatement*)node->Get().front();

		auto uses = node->GetUses();
		for(auto use : uses) {
			if(SSAStatement *stmt = dynamic_cast<SSAStatement*>(use)) {
				stmt->Replace(node, solo_member);
			}
		}

		node->Parent->RemoveStatement(*node);
		node->Unlink();
		node->Dispose();
		delete node;
	}

	void RemoveDominatedPhiNodes(SSABlock *block) const
	{
		auto nodes = GetDominatedPhiNodes(block);
		for(auto node : nodes) {
			RemoveDominatedPhiNode(node);
		}
	}

	void MovePhiNodesToStartOfBlock(SSABlock *block) const
	{
		std::list<SSAPhiStatement*> phinodes;
		for(auto stmt : block->GetStatements()) {
			if(auto phinode = dynamic_cast<SSAPhiStatement*>(stmt)) {
				phinodes.push_back(phinode);
			}
		}

		for(auto phinode : phinodes) {
			for(auto member : phinode->Get()) {
				SSAStatement *member_stmt = (SSAStatement*)member;
				GASSERT(member_stmt->Parent != phinode->Parent);
			}

			phinode->Parent->RemoveStatement(*phinode);
			phinode->Parent->AddStatement(*phinode, *phinode->Parent->GetStatements().front());
		}
	}

	bool Run(SSAFormAction& action) const override
	{
		for(auto block : action.Blocks) {
			RemoveDominatedPhiNodes(block);
			MovePhiNodesToStartOfBlock(block);
		}
		return false;
	}

};

RegisterComponent0(SSAPass, PhiCleanupPass, "PhiCleanup", "Move phi nodes to the top of blocks and remove phi nodes which refer to statements in the same block")