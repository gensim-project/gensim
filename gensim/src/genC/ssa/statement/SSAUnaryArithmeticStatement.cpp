/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
