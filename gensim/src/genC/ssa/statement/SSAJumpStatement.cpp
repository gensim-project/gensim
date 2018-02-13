/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

bool SSAJumpStatement::IsFixed() const
{
	return true;
}

std::set<SSASymbol *> SSAJumpStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSAJumpStatement::SetTarget(SSABlock* newtarget)
{
	if(Target() != nullptr) {
		Target()->RemovePredecessor(*Parent);
	}
	SetOperand(0, newtarget);
	if(Target() != nullptr) {
		Target()->AddPredecessor(*Parent);
	}
}

void SSAJumpStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitJumpStatement(*this);
}
