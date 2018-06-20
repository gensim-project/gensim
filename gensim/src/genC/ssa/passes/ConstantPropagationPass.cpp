/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/analysis/SSADominance.h"
#include "genC/ssa/statement/SSACallStatement.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSAReadStructMemberStatement.h"
#include "genC/ssa/statement/SSAStatement.h"

#include "genC/ssa/printers/SSAActionPrinter.h"

#include <typeinfo>
#include <typeindex>

using namespace gensim::genc::ssa;
#define DPRINTF(...)

/**
 Propagate constant values through variable writes/reads.

 If we have a write of a constant value to a variable, e.g.
 uint32 i
 ...
 i = 10;

 then all fully dominated reads of that variable are replaced by the constant

 j = i; // replaced
 if(some_expression) {
	k = i; // replaced
	i = 11;
 }
 l = i; // not replaced

 */

class ConstantPropagationPass : public SSAPass
{
public:
	virtual ~ConstantPropagationPass()
	{

	}
	bool Run(SSAFormAction& action) const override
	{
		DPRINTF("Starting CPP on %s\n", action.GetPrototype().GetIRSignature().GetName().c_str());
		printers::SSAActionPrinter printer(action);
		DPRINTF("%s\n", printer.ToString().c_str());
		std::vector<SSAVariableWriteStatement*> candidates;

		for(auto block : action.GetBlocks()) {
			for(auto statement : block->GetStatements()) {
				DPRINTF("Checking %s for candidacy\n", statement->GetName().c_str());
				if(IsACandidate(statement)) {
					DPRINTF(" - added to candidate list\n");
					candidates.push_back((SSAVariableWriteStatement*)statement);
				}
			}
		}

		bool changed = false;
		for(auto i : candidates) {
			changed |= RunOnStatement(&action, i);
		}

		return changed;
	}

private:
	// Returns true if the given statement is a candidate for this optimisation
	bool IsACandidate(SSAStatement *statement) const
	{
		if(SSAVariableWriteStatement *write = dynamic_cast<SSAVariableWriteStatement*>(statement)) {

			for(auto use : ((SSAValue*)write->Target())->GetUses()) {
				// are there other writes to the variable?
				if(SSAVariableKillStatement *kill = dynamic_cast<SSAVariableKillStatement*>(use)) {
					if(kill != statement) {
						DPRINTF("Found other writes to target!\n");
						return false;
					}
				}
				// is the variable passed by reference?
				if(dynamic_cast<SSACallStatement*>(use)) {
					DPRINTF("Found a use by reference!\n");
					return false;
				}
			}

			SSAStatement *expression = write->Expr();
			if(dynamic_cast<SSAConstantStatement*>(expression) || dynamic_cast<SSAReadStructMemberStatement*>(expression)) {
				return true;
			} else {
				DPRINTF("Expression was not a constant!\n");
				return false;
			}
		} else {
			DPRINTF("Tested statement was not a write!\n");
			return false;
		}
	}

	// Runs this optimisation on this statement. Returns true if a change was
	// made
	bool RunOnStatement(SSAFormAction *action, SSAVariableWriteStatement *statement) const
	{
		assert(dynamic_cast<SSAVariableWriteStatement*>(statement));

		DPRINTF("Assessed %s for constant prop\n", statement->GetName().c_str());

		// check each use of the symbol this write targets
		auto uses = ((SSAValue*)statement->Target())->GetUses();

		analysis::SSADominance dominancecalc;
		auto dominance = dominancecalc.Calculate(action);
		DPRINTF("Dominance: %s\n", dominancecalc.Print(dominance).c_str());

		SSAStatement *cst = (SSAStatement*)statement->Expr();

		bool changed = false;

		// for each use, check to see if it is fully dominated by this write
		for(auto use_value : uses) {
			SSAVariableReadStatement *use = dynamic_cast<SSAVariableReadStatement*>(use_value);
			if(use) {
				if(use->Parent != statement->Parent) {
					auto dominators = dominance.at(use->Parent);
					if(dominators.count(statement->Parent)) {
						// the use is strictly dominated, so replace it with
						// the constant value

						DPRINTF(" - %s is dominated\n", use->GetName().c_str());
						if(SSAConstantStatement *constant = dynamic_cast<SSAConstantStatement*>(cst)) {
							ReplaceUse(constant, use);
						} else if(SSAReadStructMemberStatement *str = dynamic_cast<SSAReadStructMemberStatement*>(cst)) {
							ReplaceUse(str, use);
						}
						changed = true;
					}
				} else {
					// do nothing for now
				}
			}
		}

		return changed;
	}

	bool ReplaceUse(SSAReadStructMemberStatement *rstruct, SSAVariableReadStatement *read) const
	{
		SSAReadStructMemberStatement *newread = new SSAReadStructMemberStatement(read->Parent, rstruct->Target(), rstruct->MemberName, rstruct->Index, read);
		newread->SetDiag(read->GetDiag());

		DPRINTF("Replacing struct read\n");

		auto uses = read->GetUses();
		while(!uses.empty()) {
			auto _use = *uses.begin();
			uses.erase(uses.begin());

			SSAStatement *use = dynamic_cast<SSAStatement*>(_use);
			if(use != nullptr) {
				bool replaced = use->Replace(read, newread);
				DPRINTF("%s was %s\n", use->GetName().c_str(), replaced?"replaced":"not replaced");
			} else {
				//throw std::logic_error("Unknown use type");
			}
		}

		read->Parent->RemoveStatement(*read);
		read->Dispose();
		delete read;

		return true;
	}

	bool ReplaceUse(SSAConstantStatement *constant, SSAVariableReadStatement *read) const
	{
		assert(std::find(read->Parent->GetStatements().begin(), read->Parent->GetStatements().end(), read) != read->Parent->GetStatements().end());

		DPRINTF("Replacing constant\n");

		SSAConstantStatement *new_constant = new SSAConstantStatement(read->Parent, constant->Constant, constant->GetType(), read);
		new_constant->SetDiag(read->GetDiag());

		auto uses = read->GetUses();
		while(!uses.empty()) {
			auto _use = *uses.begin();
			uses.erase(uses.begin());

			SSAStatement *use = dynamic_cast<SSAStatement*>(_use);
			if(use != nullptr) {
				bool replaced = use->Replace(read, new_constant);
				DPRINTF("%s was %s\n", use->GetName().c_str(), replaced?"replaced":"not replaced");
			} else {
				//throw std::logic_error("Unknown use type");
			}
		}

		read->Parent->RemoveStatement(*read);
		read->Dispose();
		delete read;

		return true;
	}
};

RegisterComponent0(SSAPass, ConstantPropagationPass, "ConstantPropagation", "Propagate constants through variable writes/reads")
