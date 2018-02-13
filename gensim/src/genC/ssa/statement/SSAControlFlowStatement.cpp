/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
