/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ir/IR.h"

using namespace gensim::genc::ssa;

bool SSAConstantStatement::IsFixed() const
{
	return true;
}

bool SSAConstantStatement::HasSideEffects() const
{
	return false;
}

bool SSAConstantStatement::Resolve(DiagnosticContext& ctx)
{
	if(IRType::Cast(Constant, GetType(), GetType()) != Constant) {
		ctx.Error("Internal error: a constant value doesn't fit into its own type", GetDiag());
		return false;
	}

	return true;
}

std::set<SSASymbol *> SSAConstantStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSAConstantStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitConstantStatement(*this);
}

SSAConstantStatement::SSAConstantStatement(SSABlock* parent, const IRConstant &value, const IRType& type, SSAStatement* before)
	: SSAStatement(Class_Unknown, 0, parent, before), _constant_type(type), Constant(value)
{

}
