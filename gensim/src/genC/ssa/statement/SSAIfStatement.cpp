/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAIfStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

bool SSAIfStatement::IsFixed() const
{
	return Expr()->IsFixed();
}

bool SSAIfStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	success &= SSAStatement::Resolve(ctx);
	success &= Expr()->Resolve(ctx);
	return success;
}

std::set<SSASymbol *> SSAIfStatement::GetKilledVariables()
{
	return {};
}

void SSAIfStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitIfStatement(*this);
}

SSAControlFlowStatement::target_list_t SSAIfStatement::GetTargets()
{
	if(TrueTarget() == FalseTarget()) {
		return {TrueTarget()};
	} else {
		return {TrueTarget(), FalseTarget()};
	}
}

SSAControlFlowStatement::target_const_list_t SSAIfStatement::GetTargets() const
{
	if(TrueTarget() == FalseTarget()) {
		return {TrueTarget()};
	} else {
		return {TrueTarget(), FalseTarget()};
	}
}
