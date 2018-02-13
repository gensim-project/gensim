/*
 * SSADeviceReadStatement.cpp
 *
 *  Created on: 22 May 2015
 *      Author: harry
 */

#include "genC/ssa/statement/SSADeviceReadStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/SSASymbol.h"

using namespace gensim;
using namespace gensim::genc::ssa;

SSADeviceReadStatement::SSADeviceReadStatement(SSABlock *parent, SSAStatement *dev_expr, SSAStatement *addr_expr, SSASymbol *target)
	: SSAVariableKillStatement(2, parent, target, NULL)
{
	SetDevice(dev_expr);
	SetAddress(addr_expr);
}

SSADeviceReadStatement::~SSADeviceReadStatement()
{
}

bool SSADeviceReadStatement::IsFixed() const
{
	return false;
}

void SSADeviceReadStatement::PrettyPrint(std::ostringstream & str) const
{
	str << "???";
}


std::set<SSASymbol *> SSADeviceReadStatement::GetKilledVariables()
{
	return { Target() };
}

void SSADeviceReadStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitDeviceReadStatement(*this);
}
