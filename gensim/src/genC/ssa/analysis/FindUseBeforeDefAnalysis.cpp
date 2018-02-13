/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"
#include "genC/ssa/analysis/FindUseBeforeDefAnalysis.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSACallStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"

using namespace gensim::genc::ssa;

static FindUseBeforeDefAnalysis::use_before_def_list_t FindUsesBeforeDefs(const SSABlock *block, std::set<const SSABlock*> encountered_blocks, std::set<const SSASymbol*> defd_symbols)
{
	if(encountered_blocks.count(block)) {
		return {};
	}
	encountered_blocks.insert(block);

	FindUseBeforeDefAnalysis::use_before_def_list_t ubds;

	for(auto stmt : block->GetStatements()) {
		// variable kills are defs
		if(SSAVariableKillStatement *kill = dynamic_cast<SSAVariableKillStatement*>(stmt)) {
			defd_symbols.insert(kill->Target());
		}

		// calls by reference are uses, and then defs
		if(SSACallStatement *call = dynamic_cast<SSACallStatement*>(stmt)) {
			for(unsigned i = 0; i < call->ArgCount(); ++i) {
				if(call->Target()->GetPrototype().ParameterTypes()[i].Reference) {
					GASSERT(dynamic_cast<SSASymbol*>(call->Arg(i)));

					if(defd_symbols.count((SSASymbol*)call->Arg(i)) == 0) {
						ubds.insert(call);
					}
				}
			}
		}

		// variable reads are uses
		if(SSAVariableReadStatement *read = dynamic_cast<SSAVariableReadStatement*>(stmt)) {
			if(defd_symbols.count(read->Target()) == 0) {
				ubds.insert(read);
			}
		}
	}

	// recurse into successors
	for(auto succ : block->GetSuccessors()) {
		for(auto ubd : FindUsesBeforeDefs(succ, encountered_blocks, defd_symbols)) {
			ubds.insert(ubd);
		}
	}
	return ubds;
}

FindUseBeforeDefAnalysis::use_before_def_list_t FindUseBeforeDefAnalysis::FindUsesBeforeDefs(const SSAFormAction* action)
{
	std::set<const SSASymbol *> defd_symbols;
	for(auto param : action->ParamSymbols) {
		defd_symbols.insert(param);
	}

	return ::FindUsesBeforeDefs(action->EntryBlock, {}, defd_symbols);
}
