/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSASwitchStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ir/IR.h"
#include "genC/Parser.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

bool SSASwitchStatement::IsFixed() const
{
	return Expr()->IsFixed();
}

bool SSASwitchStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	success &= SSAStatement::Resolve(ctx);
	success &= Expr()->Resolve(ctx);

	for (auto i : GetValues()) {
		if (!i.first->IsFixed()) {
			ctx.Error("Switch case values must be const at point of use", GetDiag());
			success = false;
		}
		success &= i.first->Resolve(ctx);
	}
	return success;
}

std::set<SSASymbol *> SSASwitchStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

SSAControlFlowStatement::target_const_list_t SSASwitchStatement::GetTargets() const
{
	std::set<const SSABlock*> targets;
	for(unsigned i = 0; i < OperandCount(); ++i) {
		auto operand = GetOperand(i);
		const SSABlock *block = dynamic_cast<const SSABlock*>(operand);
		if(block != nullptr) {
			targets.insert(block);
		}
	}

	SSAControlFlowStatement::target_const_list_t v;
	for(auto i : targets) {
		v.push_back(i);
	}
	return v;
}

SSAControlFlowStatement::target_list_t SSASwitchStatement::GetTargets()
{
	std::set<SSABlock*> targets;
	for(unsigned i = 1; i < OperandCount(); i += 2) {
		auto operand = GetOperand(i);

		GASSERT(dynamic_cast<SSABlock*>(operand) != nullptr);

		SSABlock *block = static_cast<SSABlock*>(operand);
		targets.insert(block);
	}

	SSAControlFlowStatement::target_list_t v;
	for(auto i : targets) {
		v.push_back(i);
	}
	return v;
}

void SSASwitchStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitSwitchStatement(*this);
}

SSASwitchStatement::~SSASwitchStatement()
{

}

const SSASwitchStatement::ValueMap SSASwitchStatement::GetValues() const
{
	ValueMap values;
	for(unsigned i = 2; i < OperandCount(); i += 2) {
		values[(SSAStatement*)GetOperand(i)] = (SSABlock*)GetOperand(i+1);
	}
	return values;
}
