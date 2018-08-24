/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/Parser.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSAPhiStatement::SSAPhiStatement(SSABlock* parent, const SSAType &type, SSAStatement* before) : SSAStatement(Class_Unknown, 0, parent, before), type_(type)
{

}

SSAPhiStatement::~SSAPhiStatement()
{
}

bool SSAPhiStatement::Add(SSAStatement* member)
{
	GASSERT(member->GetType() == type_);

	for(unsigned i = 0; i < OperandCount(); ++i) {
		if(GetOperand(i) == member) {
			return false;
		}
	}

	AddOperand(member);

	return true;
}



const SSAStatement::operand_list_t& SSAPhiStatement::Get() const
{
	CheckDisposal();
	return GetOperands();
}

void SSAPhiStatement::Set(const operand_list_t& ops)
{
	for(unsigned i = 0; i < OperandCount(); ++i) {
		SetOperand(i, nullptr);
	}
	SetOperandCount(ops.size());
	for(unsigned i = 0; i < OperandCount(); ++i) {
		SetOperand(i, ops[i]);
	}
}

unsigned int SSAPhiStatement::Count() const
{
	return OperandCount();
}

std::set<SSASymbol*> SSAPhiStatement::GetKilledVariables()
{
	return std::set<SSASymbol*>();
}

bool SSAPhiStatement::IsFixed() const
{
	for(const auto i : Get()) {
		if(!((SSAStatement*)i)->IsFixed()) return false;
	}
	return true;
}

bool SSAPhiStatement::HasSideEffects() const
{
	return false;
}


void SSAPhiStatement::PrettyPrint(std::ostringstream& str) const
{
	str << GetName() << "= phi(";

	bool first = true;
	for(const auto i : Get()) {
		if(!first) {
			str << ", ";
		}
		str << i->GetName();

		first = false;
	}

	str << ")";
}

bool SSAPhiStatement::Resolve(DiagnosticContext& ctx)
{
	return SSAStatement::Resolve(ctx);
}

void SSAPhiStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitPhiStatement(*this);
}

const SSAType SSAPhiStatement::GetType() const
{
	return type_;
}
