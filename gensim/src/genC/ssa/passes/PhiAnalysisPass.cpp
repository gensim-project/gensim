/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <typeinfo>

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
#include "genC/ssa/analysis/VariableUseDefAnalysis.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "genC/ssa/analysis/SSADominance.h"

using namespace gensim::genc::ssa;

class PhiAnalysisPass : public SSAPass
{
public:
	PhiAnalysisPass() {}
	~PhiAnalysisPass() {}

	bool IsValidToReplace(SSASymbol *symbol) const
	{
		// to be valid to replace, this symbol must only be used by variable
		// reads and writes (no variable kills)
		// it must also have at least one read
		// it also cannot be a parameter
		if(symbol->SType == gensim::genc::Symbol_Parameter) {
			return false;
		}
		if(symbol->GetType().Reference) {
			return false;
		}

		int reads = 0;
		for(SSAValue *use : ((SSAValue*)(symbol))->GetUses()) {
			if(dynamic_cast<SSAVariableReadStatement*>(use)) {
				reads++;
			}
			if(dynamic_cast<SSAVariableKillStatement*>(use)) {
				if(dynamic_cast<SSAVariableWriteStatement*>(use)) {
					continue;
				} else {
					return false;
				}
			}
		}

		return reads != 0;
	}

	bool RunOnBlock(SSABlock *block, std::set<SSABlock*> &blocks, std::map<SSASymbol *, SSAStatement *> live_values, std::map<std::pair<SSABlock*, SSASymbol*>, SSAPhiStatement*> &phi_statements, const std::set<SSASymbol*> &valid_variables) const
	{
		bool changed = false;
		bool infochanged = false;
		bool newblock = blocks.count(block) == 0;
		blocks.insert(block);

		std::set<SSASymbol *> read_symbols;
		// look through the block for variable reads
		for(auto stmt : block->GetStatements()) {
			if(SSAVariableReadStatement *read = dynamic_cast<SSAVariableReadStatement*>(stmt)) {
				bool spans = false;
				for(auto use : ((SSAValue*)read->Target())->GetUses()) {
					if(SSAStatement *stmt = dynamic_cast<SSAStatement*>(use)) {
						if(stmt->Parent != block) {
							spans = true;
						}
					}
				}

				if(spans) {
					read_symbols.insert(read->Target());
				}
			}
		}

		// insert a phi node for every live value which is read. If a phi statement already
		// exists for this block/symbol, then add to it.
		for(auto live_value : read_symbols) {
			if(live_values.count(live_value)) {
				SSAPhiStatement *&statement = phi_statements[std::make_pair(block, live_value)];
				if(statement == nullptr) {
					statement = new SSAPhiStatement(block, live_value->GetType(), block->GetStatements().front());
					new SSAVariableWriteStatement(block, live_value, statement, block->GetStatements().at(1));
				}
				if(statement->Add(live_values[live_value])) {
					changed = true;
//					std::cout << "Added " << live_values[live_value]->GetName() << " to " << statement->GetName() << std::endl;
				}
				GASSERT(statement->Get().size());
			}
		}

		// now, iterate through the block, updating live values
		for(auto stmt : block->GetStatements()) {
			if(SSAVariableWriteStatement *writestmt = dynamic_cast<SSAVariableWriteStatement*>(stmt)) {
				if(valid_variables.count(writestmt->Target())) {
					if(live_values[writestmt->Target()] != writestmt->Expr()) {
						live_values[writestmt->Target()] = writestmt->Expr();
						infochanged = true;
					}
				}
			}
		}

		for(auto i : block->GetSuccessors()) {
			changed |= RunOnBlock(i, blocks, live_values, phi_statements, valid_variables);
		}

		return changed;
	}

	bool Run(SSAFormAction& action) const override
	{
		SSAPass *renamer = GetComponent<SSAPass>("ParameterRename");
		renamer->Run(action);

		LoopAnalysis la;
		if(la.Analyse(action).LoopExists) {
			return false;
		}

		std::set<SSASymbol *> valid_variables;
		for(auto s : action.Symbols()) {
			if(IsValidToReplace(s)) {
				valid_variables.insert(s);
			}
		}

		// If we found no valid variables, then just return
		if(valid_variables.empty()) {
			return false;
		} else {
			// Otherwise, run phi analysis, starting with the entry block
			std::map<std::pair<SSABlock*, SSASymbol*>, SSAPhiStatement*> phi_statements;
			std::set<SSABlock*> blocks;

			while(RunOnBlock(action.EntryBlock, blocks, {}, phi_statements, valid_variables)) ;

			return false;
		}
	}
};

RegisterComponent0(SSAPass, PhiAnalysisPass, "PhiAnalysis", "Perform phi analysis");