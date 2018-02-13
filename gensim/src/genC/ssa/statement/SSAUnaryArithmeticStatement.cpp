/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAUnaryArithmeticStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

bool SSAUnaryArithmeticStatement::IsFixed() const
{
	return Expr()->IsFixed();
}

bool SSAUnaryArithmeticStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	success &= SSAStatement::Resolve(ctx);
	success &= Expr()->Resolve(ctx);
	return success;
}

std::set<SSASymbol *> SSAUnaryArithmeticStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

SSAUnaryArithmeticStatement::~SSAUnaryArithmeticStatement()
{
}

bool SSAUnaryArithmeticStatement::HasSideEffects() const
{
	return false;
}

void SSAUnaryArithmeticStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitUnaryArithmeticStatement(*this);
}
