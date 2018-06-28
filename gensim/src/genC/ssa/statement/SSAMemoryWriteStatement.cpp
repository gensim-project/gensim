/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAMemoryWriteStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSAMemoryWriteStatement::~SSAMemoryWriteStatement()
{
}

SSAMemoryWriteStatement &SSAMemoryWriteStatement::CreateWrite(SSABlock *parent, SSAStatement *addrExpr, SSAStatement *valueExpr, uint8_t width, const gensim::arch::MemoryInterfaceDescription *interface)
{
	SSAMemoryWriteStatement &stmt = *(new SSAMemoryWriteStatement(parent, addrExpr, valueExpr, width, interface));
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
