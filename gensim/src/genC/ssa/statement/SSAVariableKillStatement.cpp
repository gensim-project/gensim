/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

SSAVariableKillStatement::SSAVariableKillStatement(int extra_operands, SSABlock *_parent, SSASymbol *_target, SSAStatement *_before)
	: SSAStatement(Class_Unknown, extra_operands+1, _parent, _before)
{
	GASSERT(_target);
	GASSERT(_target->GetType() != IRTypes::Void);
	SetTarget(_target);
}

SSAVariableKillStatement::~SSAVariableKillStatement()
{
}

void SSAVariableKillStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVariableKillStatement(*this);
}
