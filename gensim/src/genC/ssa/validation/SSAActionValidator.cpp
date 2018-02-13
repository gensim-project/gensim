/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/validation/SSAActionValidator.h"
#include "genC/ssa/validation/SSAActionValidationPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "ComponentManager.h"

using namespace gensim::genc::ssa::validation;

SSAActionValidator::SSAActionValidator()
{
	for(auto i : GetRegisteredComponentNames<SSAActionValidationPass>()) {
		passes_.push_back(GetComponent<SSAActionValidationPass>(i));
	}
}

bool SSAActionValidator::Run(SSAFormAction* action, DiagnosticContext& diag)
{
	// validate every statement in the action, then the action itself
	for(auto block : action->Blocks) {
		for(auto stmt : block->GetStatements()) {
			if(!statement_validator_.Run(stmt, diag)) {
				diag.Error("Statement " + stmt->GetName() + " failed validation");
				return false;
			}
		}
	}
	
	for(auto i : passes_) {
		if(!i->Run(action, diag)) {
			return false;
		}
	}
	return true;
}
