/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "ComponentManager.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

bool SSAStatementValidationPass::Run(SSAStatement* stmt, DiagnosticContext& ctx)
{
	success_ = true;

	diag_ = &ctx;

	stmt->Accept(*this);

	return success_;
}

void SSAStatementValidationPass::Fail(const std::string message)
{
	Fail(message, DiagNode());
}

void SSAStatementValidationPass::Fail(const std::string& message, const DiagNode& diag)
{
	diag_->AddEntry(DiagnosticClass::Error, message, diag);
	success_ = false;
}

void SSAStatementValidationPass::Assert(bool expression, const std::string& message, const DiagNode& diag)
{
	if(!expression) {
		Fail(message, diag);
	}
}


gensim::DiagnosticContext& SSAStatementValidationPass::Diag()
{
	return *diag_;
}

void SSAStatementValidationPass::VisitStatement(SSAStatement& stmt)
{
	UNIMPLEMENTED;
}

DefineComponentType0(SSAStatementValidationPass);