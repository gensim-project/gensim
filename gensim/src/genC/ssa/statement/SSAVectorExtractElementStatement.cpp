/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAVectorExtractElementStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/SSASymbol.h"

using namespace gensim;
using namespace gensim::genc::ssa;

const SSAType SSAVectorExtractElementStatement::ResolveType(const SSAStatement& base) const
{
	SSAType output_type = base.GetType();
	output_type.VectorWidth = 1;
	return output_type;
}

SSAVectorExtractElementStatement::SSAVectorExtractElementStatement(SSABlock *parent, SSAStatement *base_expr, SSAStatement *index_expr)
	: SSAStatement(Class_Unknown, 2, parent, NULL)
{
	SetBase(base_expr);
	SetIndex(index_expr);
}

SSAVectorExtractElementStatement::~SSAVectorExtractElementStatement()
{

}

void SSAVectorExtractElementStatement::PrettyPrint(std::ostringstream &str) const
{
	str << Base()->GetName();
	str << "[";
	str << Index()->GetName();
	str << "]";
}

std::set<SSASymbol *> SSAVectorExtractElementStatement::GetKilledVariables()
{
	return {};
}

bool SSAVectorExtractElementStatement::IsFixed() const
{
	return Index()->IsFixed() && Base()->IsFixed();
}

bool SSAVectorExtractElementStatement::HasSideEffects() const
{
	return false;
}


bool SSAVectorExtractElementStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	success &= Index()->Resolve(ctx);
	success &= Base()->Resolve(ctx);

	Resolved = true;

	return true;
}

void SSAVectorExtractElementStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVectorExtractElementStatement(*this);
}
