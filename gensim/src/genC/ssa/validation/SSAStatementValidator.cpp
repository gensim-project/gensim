/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/validation/SSAStatementValidator.h"
#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "ComponentManager.h"

using namespace gensim::genc::ssa::validation;

SSAStatementValidator::SSAStatementValidator()
{
	for(auto i : GetRegisteredComponentNames<SSAStatementValidationPass>()) {
		passes_.push_back(GetComponent<SSAStatementValidationPass>(i));
	}
}

bool SSAStatementValidator::Run(SSAStatement* stmt, DiagnosticContext& diag)
{
	for(auto i : passes_) {
		if(!i->Run(stmt, diag)) {
			return false;
		}
	}
	return true;
}
