/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "genC/ir/IRAction.h"
#include "genC/ssa/statement/SSARegisterStatement.h"
#include "genC/Parser.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

bool SSARegisterStatement::IsFixed() const
{
	if (!IsRead) {
		if (IsBanked && Value()->IsFixed() && RegNum()->IsFixed()) return true;
		if (!IsBanked && Value()->IsFixed()) return true;
	}
	return false;
}

SSAType SSARegisterStatement::ResolveType(const gensim::arch::ArchDescription& arch, bool is_banked, uint8_t bank)
{
	if (is_banked)
		return arch.GetRegFile().GetBankByIdx(bank).GetRegisterIRType();
	else
		return arch.GetRegFile().GetSlotByIdx(bank).GetIRType();
}

bool SSARegisterStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	success &= SSAStatement::Resolve(ctx);
	if (IsBanked) success &= RegNum()->Resolve(ctx);
	if (!IsRead) success &= Value()->Resolve(ctx);
	return success;
}

SSARegisterStatement *SSARegisterStatement::CreateBankedRead(SSABlock *parent, uint8_t bank, SSAStatement *regNumExpr)
{
	GASSERT(regNumExpr->HasValue());

	SSARegisterStatement *stmt = new SSARegisterStatement(parent, bank, regNumExpr, NULL);
	stmt->IsBanked = true;
	stmt->IsRead = true;
	return stmt;
}

SSARegisterStatement *SSARegisterStatement::CreateBankedWrite(SSABlock *parent, uint8_t bank, SSAStatement *regNumExpr, SSAStatement *valueExpr)
{
	GASSERT(valueExpr->HasValue());
	GASSERT(regNumExpr->HasValue());

	SSARegisterStatement *stmt = new SSARegisterStatement(parent, bank, regNumExpr, valueExpr);
	stmt->IsBanked = true;
	stmt->IsRead = false;
	return stmt;
}

SSARegisterStatement *SSARegisterStatement::CreateRead(SSABlock *parent, uint8_t reg)
{
	SSARegisterStatement *stmt = new SSARegisterStatement(parent, reg, NULL, NULL);
	stmt->IsBanked = false;
	stmt->IsRead = true;
	return stmt;
}

SSARegisterStatement *SSARegisterStatement::CreateWrite(SSABlock *parent, uint8_t reg, SSAStatement *valueExpr)
{
	GASSERT(valueExpr->HasValue());

	SSARegisterStatement *stmt = new SSARegisterStatement(parent, reg, NULL, valueExpr);
	stmt->IsBanked = false;
	stmt->IsRead = false;
	return stmt;
}

std::set<SSASymbol *> SSARegisterStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

SSARegisterStatement::~SSARegisterStatement()
{

}

void SSARegisterStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitRegisterStatement(*this);
}
