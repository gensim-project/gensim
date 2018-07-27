/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/passes/SSAPass.h"

#include "genC/ssa/statement/SSAControlFlowStatement.h"

#include <list>

using namespace gensim::genc::ssa;

class BlockMergingPass : public SSAPass
{
public:
	virtual ~BlockMergingPass()
	{

	}

	bool Run(SSAFormAction& action) const override
	{
		// naive implementation: simply loop through all of the blocks. If we change a block, loop
		// through all of them again
		bool block_changed = true;
		bool change_made = false;

		while (block_changed) {
			block_changed = false;
			for (auto b : action.GetBlocks()) {
				// A block can be merged with its successor if it has one successor, and that block has
				// only one predecessor. The successor cannot be the entry block (since that has an
				// additional implicit predecessor) or the block we started with (self loop)
				if (b->GetSuccessors().size() == 1 && b->GetSuccessors().front()->GetPredecessors().size() == 1 && b->GetSuccessors().front() != action.EntryBlock && b->GetSuccessors().front() != b) {
					// this block can be merged
					// to merge a block, first remove the control flow instruction from the end of the
					// parent block. Then, insert the instructions from the child block at the end of the
					// parent block
					SSABlock *child = b->GetSuccessors().front();

					// so: first delete the control flow instruction of the parent block
					SSAControlFlowStatement *ctrlflow = b->GetControlFlow();
					if (ctrlflow) {
						b->RemoveStatement(*ctrlflow);
						// fprintf(stderr, "Deleting control flow instruction %p due to block merging of %p with %p\n", ctrlflow, b, child);
						ctrlflow->Dispose();
						delete ctrlflow;
					}

					// now insert the child block into the end of the parent block
					while (child->GetStatements().size() > 1) { // we need to handle the control flow insn specially
						SSAStatement *stmt = child->GetStatements().front();
						assert(!dynamic_cast<SSAControlFlowStatement *>(stmt));
						child->RemoveStatement(*stmt);
						stmt->Parent = b;
						b->AddStatement(*stmt);
					}

					if(child->GetControlFlow()) {
						child->GetControlFlow()->Move(b);
					}

					// now delete the child block
					action.RemoveBlock(child);

					while (child->GetSuccessors().size()) {
						SSABlock *succ = child->GetSuccessors().front();
						child->GetControlFlow()->Replace(succ, NULL);
					}

					child->Dispose();
					delete child;

					block_changed = true;
					change_made = true;
					break;
				}
			}
		}
		return change_made;
	}

};

RegisterComponent0(SSAPass, BlockMergingPass, "BlockMerging", "Merge trivially connected blocks together")
