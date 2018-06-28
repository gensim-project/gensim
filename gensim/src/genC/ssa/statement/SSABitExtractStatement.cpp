/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSABitExtractStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSABitExtractStatement::SSABitExtractStatement(SSABlock* parent, SSAStatement* Expr, SSAStatement* From, SSAStatement* To, SSAStatement* before) : SSAStatement(Class_Unknown, 3, parent, before)
{
	SetExpr(Expr);
	SetBitFrom(From);
	SetBitTo(To);
}

bool SSABitExtractStatement::IsFixed() const
{
	return Expr()->IsFixed() && BitFrom()->IsFixed() && BitTo()->IsFixed();
}

void SSABitExtractStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitBitExtractStatement(*this);
}
