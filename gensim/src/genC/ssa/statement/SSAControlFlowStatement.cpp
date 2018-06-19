/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAControlFlowStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

void SSAControlFlowStatement::Move(SSABlock *newParent, SSAStatement *insertBefore)
{
	// first, unlink predecessors
	std::vector<SSABlock *> targets = GetTargets();
	for (std::vector<SSABlock *>::iterator i = targets.begin(); i != targets.end(); ++i) (*i)->RemovePredecessor(*Parent);

	Parent->RemoveStatement(*this);
	Parent = newParent;
	if (insertBefore != NULL)
		newParent->AddStatement(*this, *insertBefore);
	else
		newParent->AddStatement(*this);

	for (std::vector<SSABlock *>::iterator i = targets.begin(); i != targets.end(); ++i) (*i)->AddPredecessor(*Parent);
}

void SSAControlFlowStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitControlFlowStatement(*this);
}
