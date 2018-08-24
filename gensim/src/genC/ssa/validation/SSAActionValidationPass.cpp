/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/validation/SSAActionValidationPass.h"
#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "ComponentManager.h"

using gensim::genc::ssa::validation::SSAActionValidationPass;
using gensim::genc::ssa::validation::SSAValidationManager;

SSAActionValidationPass::~SSAActionValidationPass()
{

}


bool SSAValidationManager::Run(SSAFormAction* action, DiagnosticContext &ctx)
{
	if(action_passes_.empty()) {
		for(auto i : GetRegisteredComponentNames<SSAActionValidationPass>()) {
			action_passes_.push_back(GetComponent<SSAActionValidationPass>(i));
		}
	}

	for(auto block : action->GetBlocks()) {
		for(auto stmt : block->GetStatements()) {
			if(!Run(stmt, ctx)) {
				return false;
			}
		}
	}

	bool success = true;
	for(auto pass : action_passes_) {
		success &= pass->Run(action, ctx);
	}
	return success;
}

bool SSAValidationManager::Run(SSAStatement* stmt, DiagnosticContext &ctx)
{
	if(statement_passes_.empty()) {
		for(auto i : GetRegisteredComponentNames<SSAStatementValidationPass>()) {
			statement_passes_.push_back(GetComponent<SSAStatementValidationPass>(i));
		}
	}

	bool success = true;
	for(auto pass : statement_passes_) {
		success &= pass->Run(stmt, ctx);
	}
	return success;
}


DefineComponentType0(SSAActionValidationPass);
