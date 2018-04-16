/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAMemoryReadStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSAMemoryReadStatement::~SSAMemoryReadStatement()
{
}

bool SSAMemoryReadStatement::IsFixed() const
{
	return Fixed;
}

bool SSAMemoryReadStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	if (Target()->IsReference()) {
		Replace(Target(), Target()->GetReferencee());
	}

	if(Target()->GetType().ElementSize() != Width) {
		ctx.Error("Memory access target size must match the width of the memory access", GetDiag());
		success = false;
	}

	success &= SSAStatement::Resolve(ctx);
	success &= Addr()->Resolve(ctx);
	return success;
}

SSAMemoryReadStatement &SSAMemoryReadStatement::CreateRead(SSABlock *parent, SSAStatement *addrExpr, SSASymbol *target, uint8_t width, bool sign, const gensim::arch::MemoryInterfaceDescription *interface)
{
	SSAMemoryReadStatement &stmt = *(new SSAMemoryReadStatement(parent, addrExpr, target, width, sign, interface));
	return stmt;
}

std::set<SSASymbol *> SSAMemoryReadStatement::GetKilledVariables()
{
	return {Target()};
}

void SSAMemoryReadStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitMemoryReadStatement(*this);
}
