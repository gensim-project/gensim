/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAVectorInsertElementStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/SSASymbol.h"

using namespace gensim;
using namespace gensim::genc::ssa;

SSAVectorInsertElementStatement::SSAVectorInsertElementStatement(SSABlock *parent, SSAStatement *initial, SSAStatement *index_expr, SSAStatement *value_expr)
	: SSAStatement(Class_Unknown, 3, parent)
{
	SetBase(initial);
	SetIndex(index_expr);
	SetValue(value_expr);
}

SSAVectorInsertElementStatement::~SSAVectorInsertElementStatement()
{

}

void SSAVectorInsertElementStatement::PrettyPrint(std::ostringstream &str) const
{
	str << GetName() << " = ";
	str << Base()->GetName() << "[";
	str << Index()->GetName();
	str << "] <= " << Value()->GetName();
}

bool SSAVectorInsertElementStatement::IsFixed() const
{
	return Base()->IsFixed() && Index()->IsFixed() &&  Value()->IsFixed() && (Parent->IsFixed() != BLOCK_NEVER_CONST);
}

std::set<SSASymbol *> SSAVectorInsertElementStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSAVectorInsertElementStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVectorInsertElementStatement(*this);
}
