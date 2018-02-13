/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAMemoryWriteStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSAMemoryWriteStatement::~SSAMemoryWriteStatement()
{
}

SSAMemoryWriteStatement &SSAMemoryWriteStatement::CreateWrite(SSABlock *parent, SSAStatement *addrExpr, SSAStatement *valueExpr, uint8_t width, bool user)
{
	SSAMemoryWriteStatement &stmt = *(new SSAMemoryWriteStatement(parent, addrExpr, valueExpr, width, user));
	return stmt;
}

bool SSAMemoryWriteStatement::IsFixed() const
{
	return Addr()->IsFixed() && Value()->IsFixed();
}

bool SSAMemoryWriteStatement::Resolve(DiagnosticContext &ctx)
{
	return Addr()->Resolve(ctx) && Value()->Resolve(ctx);
}

std::set<SSASymbol *> SSAMemoryWriteStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSAMemoryWriteStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitMemoryWriteStatement(*this);
}
