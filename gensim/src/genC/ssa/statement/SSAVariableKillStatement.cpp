/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

SSAVariableKillStatement::SSAVariableKillStatement(int extra_operands, SSABlock *_parent, SSASymbol *_target, SSAStatement *_before)
	: SSAStatement(Class_Unknown, extra_operands+1, _parent, _before)
{
	assert(_target);
	assert(_target->GetType() != IRTypes::Void);
	SetTarget(_target);
}

SSAVariableKillStatement::~SSAVariableKillStatement()
{
}

void SSAVariableKillStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVariableKillStatement(*this);
}
