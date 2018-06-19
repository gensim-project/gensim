/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSARaiseStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

SSARaiseStatement::SSARaiseStatement(SSABlock* parent, SSAStatement* before) : SSAControlFlowStatement(1, parent, before)
{
}

bool SSARaiseStatement::IsFixed() const
{
	return true;
}

bool SSARaiseStatement::Resolve(DiagnosticContext &ctx)
{
	return true;
}

std::set<SSASymbol *> SSARaiseStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

SSARaiseStatement::~SSARaiseStatement()
{
}

void SSARaiseStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitRaiseStatement(*this);
}
