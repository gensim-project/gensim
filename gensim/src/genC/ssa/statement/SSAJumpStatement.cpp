/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
