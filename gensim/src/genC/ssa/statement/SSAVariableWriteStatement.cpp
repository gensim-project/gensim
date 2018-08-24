/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

SSAVariableWriteStatement::SSAVariableWriteStatement(SSABlock *_parent, SSASymbol *_target, SSAStatement *_expr, SSAStatement *_before)
	: SSAVariableKillStatement(1, _parent, _target, _before)
{
	SetExpr(_expr);
}

SSAVariableWriteStatement::~SSAVariableWriteStatement()
{

}

bool SSAVariableWriteStatement::IsFixed() const
{
	return Expr()->IsFixed() && (Parent->IsFixed() != BLOCK_NEVER_CONST);
}

bool SSAVariableWriteStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	assert(!IsDisposed());

	if(Parent->Parent->Symbols().count(Target()) == 0) {
		throw std::logic_error("");
	}

	if(Expr()->GetType() != Target()->GetType()) {
		throw std::logic_error("A value written to a variable must match the type of the variable");
	}

	// assert(target->IsReference() || Parent->Parent.GetDominatingAllocs(target, this).size() != 0);
	assert(!Target()->IsReference() || Target()->GetReferencee());
	while (Target()->IsReference()) {
		Replace(Target(), Target()->GetReferencee());
	}
	success &= SSAStatement::Resolve(ctx);
	success &= Expr()->Resolve(ctx);
	return success;
}

std::set<SSASymbol *> SSAVariableWriteStatement::GetKilledVariables()
{
	if(!Expr()->IsFixed()) {
		return {Target()};
	} else {
		return {};
	}
}

void SSAVariableWriteStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVariableWriteStatement(*this);
}
