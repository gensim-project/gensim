/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/statement/SSACallStatement.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSACallStatement::SSACallStatement(SSABlock *parent, SSAActionBase *target, std::vector<SSAValue *> args)
	: SSAStatement(Class_Call, 1, parent)
{
	SetTarget(target);
	for(auto i : args) {
		AddOperand(i);
	}
}

bool SSACallStatement::IsFixed() const
{
	return false;
}

std::set<SSASymbol *> SSACallStatement::GetKilledVariables()
{
	// look through my arguments: see if any of them are references
	std::set<SSASymbol*> symbols;

	for(unsigned i = 0; i < ArgCount(); ++i) {
		auto op = Arg(i);
		SSASymbol *read = dynamic_cast<SSASymbol*>(op);
		if(read != nullptr) {
			symbols.insert(read);
		}
	}

	return symbols;
}

void SSACallStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitCallStatement(*this);
}

SSACallStatement::~SSACallStatement()
{
}
