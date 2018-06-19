/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSASelectStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

bool SSASelectStatement::IsFixed() const
{
	return Cond()->IsFixed() && TrueVal()->IsFixed() && FalseVal()->IsFixed();
}

std::set<SSASymbol *> SSASelectStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSASelectStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitSelectStatement(*this);
}
