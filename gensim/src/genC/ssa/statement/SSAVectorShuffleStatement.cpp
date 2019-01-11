/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRType.h"
#include "genC/ir/IRConstant.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAVectorShuffleStatement.h"

using namespace gensim::genc::ssa;

void SSAVectorShuffleStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVectorShuffleStatement(*this);
}

SSAVectorShuffleStatement* SSAVectorShuffleStatement::Concatenate(SSAStatement* lhs, SSAStatement* rhs)
{
	GASSERT(lhs->Parent == rhs->Parent);
	GASSERT(lhs->GetType().VectorWidth > 1 && rhs->GetType().VectorWidth > 1);
	int indices_width = lhs->GetType().VectorWidth + rhs->GetType().VectorWidth;

	IRType indices_type = IRType::Vector(IRTypes::UInt32, indices_width);
	IRConstant indices_constant = IRConstant::Vector(indices_width, IRConstant::Integer(0));
	for(int i = 0; i < indices_width; ++i) {
		indices_constant.VPut(i, IRConstant::Integer(i));
	}

	SSAConstantStatement *indices_stmt = new SSAConstantStatement(lhs->Parent, indices_constant, indices_type);
	return new SSAVectorShuffleStatement(lhs->Parent, lhs, rhs, indices_stmt);
}

std::set<SSASymbol*> SSAVectorShuffleStatement::GetKilledVariables()
{
	return {};
}

const SSAType SSAVectorShuffleStatement::GetType() const
{
	IRType vtype = LHS()->GetType();
	vtype.VectorWidth = Indices()->GetType().VectorWidth;

	return vtype;
}

SSAVectorShuffleStatement::SSAVectorShuffleStatement(SSABlock* parent, SSAStatement* lhs, SSAStatement* rhs, SSAStatement* shuffle, SSAStatement *before) : SSAStatement(StatementClass::Class_Unknown, 3, parent, before)
{
	SetLHS(lhs);
	SetRHS(rhs);
	SetIndices(shuffle);
}

SSAVectorShuffleStatement::~SSAVectorShuffleStatement()
{

}

bool SSAVectorShuffleStatement::IsFixed() const
{
	return LHS()->IsFixed() && RHS()->IsFixed() && Indices()->IsFixed();
}

void SSAVectorShuffleStatement::PrettyPrint(std::ostringstream& str) const
{
	str << "Shuffle" << std::endl;
}
