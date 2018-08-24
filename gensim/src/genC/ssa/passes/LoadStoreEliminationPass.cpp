/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAMemoryReadStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSAStatement.h"

using namespace gensim::genc::ssa;

static bool DoLoadElimination(SSABlock *block)
{
	bool elimination = false;
	std::map<SSASymbol *, SSAStatement *> LiveValues;
	std::list<SSAStatement *> DeadStatements;

	/*
	 *  This optimisation pass should eliminate multiple reads from variables within a single basic block.
	 *  A read can be removed (and its uses replaced) only if there is no 'kill' of the variable between
	 *  this read and the previous read. E.g:
	 *
	 *  Store variable_a = 10
	 *  uint32 stmt_1 = Read variable_a
	 *  uint32 stmt_2 = Read variable_a
	 *  return stmt_1 + stmt_2
	 *
	 *  can be reduced to
	 *  Store variable_a = 10
	 *  uint32 stmt_1 = Read variable_a
	 *  return stmt_1 + stmt_1
	 *
	 *  However:
	 *  Store variable_a = 10
	 *  uint32 stmt_1 = Read variable_a
	 *  LoadMem variable_a => variable_a
	 *  uint32 stmt_2 = Read variable_a
	 *  return stmt_1 + stmt_2
	 *
	 *  can not be reduced, since the LoadMem kills the variable
	 */

//		DPRINTF("Load Elim %s: %s\n", block->Parent.Action.Name.c_str(), block->GetName().c_str());

	for(auto stmt : block->GetStatements()) {
		if(auto *read = dynamic_cast<SSAVariableReadStatement*>(stmt)) {
			// statement is a read. If target is already live, then this is a dead read.
			if(LiveValues.count(read->Target())) {
				// Replace all uses of this statement with the previous load statement

				std::set<SSAStatement*> replacees;
				for(auto i : read->GetUses()) {
					SSAStatement *stmt = dynamic_cast<SSAStatement*>(i);
					if(stmt) {
						replacees.insert(stmt);
					}
				}
				for(auto i : replacees) {
					bool replaced = i->Replace(read, LiveValues[read->Target()]);
					assert(replaced);
				}

				// Mark this read as dead
				DeadStatements.push_back(read);
				elimination = true;
			} else {
				// Otherwise, mark this variable as live
				LiveValues[read->Target()] = read;
//					DPRINTF("%s marked as live with %s\n", read->Target->GetName().c_str(), read->GetName().c_str());
			}
		} else if(auto *kill = dynamic_cast<SSAVariableKillStatement*>(stmt)) {
			// If this statement is a variable write, then replace the live value with the value written to the variable
			if(auto *write = dynamic_cast<SSAVariableWriteStatement*>(stmt)) {
				LiveValues[write->Target()] = write->Expr();
//					DPRINTF("%s made live by %s (%s)\n", write->GetTarget()->GetName().c_str(), write->GetName().c_str(), write->GetExpr()->PrettyPrint().c_str());
			} else {
				// Otherwise, erase the live value
				LiveValues.erase(kill->Target());
//					DPRINTF("%s killed by %s\n", kill->GetTarget()->GetName().c_str(), kill->GetName().c_str());
			}
		}
	}

	// Eliminate the dead reads
	for(auto stmt : DeadStatements) {
		block->RemoveStatement(*stmt);
		stmt->Dispose();
		delete stmt;
	}

	return elimination;
}

static bool DoStoreElimination(SSABlock *block)
{
	bool elimination = false;
	std::list<SSAStatement *> DeadStatements;
	// loop over the block again and remove any stores which do not dominate any loads (dead store elimination)

//		DPRINTF("Store Elim %s: %s\n", block->Parent.Action.Name.c_str(), block->GetName().c_str());

	for(auto &i : block->GetStatements()) {
		if (SSAVariableWriteStatement *write = dynamic_cast<SSAVariableWriteStatement *>(i)) {
			//If this statement has a value, and that value is used, don't eliminate it
			if(write->GetUses().size()) {
				continue;
			}

			if (block->Parent->GetDominatedReads(write).size() == 0 && !write->Target()->IsReference()) {
				DeadStatements.push_back(write);
				elimination = true;
			}
		}
	}

	// finally, remove any statements we have marked as dead
	for (std::list<SSAStatement *>::iterator s_i = DeadStatements.begin(); s_i != DeadStatements.end(); ++s_i) {
		block->RemoveStatement(**s_i);
	}

	// finally finally, delete the statements
	while (!DeadStatements.empty()) {
		SSAStatement *stmt = DeadStatements.front();
		assert(stmt->GetUses().empty());
		DeadStatements.remove(stmt);
		block->RemoveStatement(*stmt);
		stmt->Dispose();
		delete stmt;
	}
	return elimination;
}

class LoadStoreEliminationPass : public SSAPass
{
public:
	virtual ~LoadStoreEliminationPass()
	{

	}

	bool Run(SSAFormAction& action) const override
	{
		bool change_made = false;
		for(auto i : action.GetBlocks()) {
			if (
			    DoLoadElimination(i)
			    ||
			    DoStoreElimination(i)
			) {
				change_made = true;  // loop until we finish eliminating things
			}

		}
		return change_made;
	}
};

RegisterComponent0(SSAPass, LoadStoreEliminationPass, "LoadStoreElimination", "Eliminate redundant loads and stores")
