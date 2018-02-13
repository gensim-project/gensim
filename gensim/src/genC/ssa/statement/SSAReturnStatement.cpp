/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAReturnStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

SSAReturnStatement::SSAReturnStatement(SSABlock* parent, SSAStatement* value, SSAStatement* before) : SSAControlFlowStatement(1, parent, before)
{
	SetOperand(0, value);
}


bool SSAReturnStatement::IsFixed() const
{
	return true;
}

bool SSAReturnStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	if (Value() != NULL) success &= Value()->Resolve(ctx);
	return success;
}

std::set<SSASymbol *> SSAReturnStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

SSAReturnStatement::~SSAReturnStatement()
{
}

void SSAReturnStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitReturnStatement(*this);
}
