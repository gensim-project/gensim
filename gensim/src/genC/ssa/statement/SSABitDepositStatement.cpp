#include "genC/ssa/statement/SSABitDepositStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSABitDepositStatement::SSABitDepositStatement(SSABlock* parent, SSAStatement* expr, SSAStatement* from_expr, SSAStatement* to_expr, SSAStatement* value_expr, SSAStatement *before)
	: SSAStatement(Class_Unknown, 4, parent, before)
{
	SetExpr(expr);
	SetBitFrom(from_expr);
	SetBitTo(to_expr);
	SetValue(value_expr);
}

bool SSABitDepositStatement::IsFixed() const
{
	return Expr()->IsFixed() && Value()->IsFixed() && BitFrom()->IsFixed() && BitTo()->IsFixed();
}

void SSABitDepositStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitBitDepositStatement(*this);
}
