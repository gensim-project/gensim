/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
