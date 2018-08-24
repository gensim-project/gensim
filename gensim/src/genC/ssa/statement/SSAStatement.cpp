/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAStatement.h"

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/printers/SSAStatementPrinter.h"

#include "genC/ir/IRAction.h"
#include "genC/ir/IR.h"

#include "genC/Parser.h"
#include "arch/ArchComponents.h"
#include "isa/ISADescription.h"

#include "DiagnosticContext.h"

#include <cassert>
#include <fcntl.h>
#include <list>
#include <set>
#include <typeinfo>

using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSAStatement::SSAStatement(StatementClass clazz, int operand_count, SSABlock* parent, SSAStatement* before)
	:	class_(clazz), SSAValue(parent->GetContext(), parent->GetContext().GetValueNamespace()),
	  Parent(parent),
	  Resolved(false)
{
	if (before == NULL) {
		Parent->AddStatement(*this);
	} else {
		Parent->AddStatement(*this, *before);
	}

#ifdef DEBUG_STATEMENTS
	_all_statements.insert(this);
#endif

	operands_.resize(operand_count);
}

std::string SSAStatement::ToString() const
{
	printers::SSAStatementPrinter stmt_printer(*this);
	return stmt_printer.ToString();
}

std::string SSAStatement::GetName() const
{
	std::ostringstream str;
	str << "s_" << Parent->GetName() << "_" << std::dec << GetIndex();

	return str.str();
}

int SSAStatement::GetIndex() const
{
	int Number = 0;
	for (auto ci = Parent->GetStatements().begin(); ci != Parent->GetStatements().end(); ++ci) {
		if (*ci == this) break;
		Number++;
	}

	return Number;
}


bool SSAStatement::Replace(const SSAValue* find, SSAValue* replace)
{
	bool found = false;
	for (unsigned i = 0; i < operands_.size(); ++i) {
		if(GetOperand(i) == find) {
			SetOperand(i, replace);
			found = true;
		}
	}

	return found;
}

void SSAStatement::SetOperand(int idx, SSAValue* new_value)
{
	CheckDisposal();

	if (operands_.at(idx) != nullptr) {
		operands_.at(idx)->RemoveUse(this);
	}

	operands_.at(idx) = new_value;

	if (operands_.at(idx) != nullptr) {
		operands_.at(idx)->AddUse(this);
	}
}

void SSAStatement::SetOperandCount(int count)
{
	if(operands_.size() > count) {
		for(unsigned i = operands_.size(); i < count; ++i) {
			SetOperand(i, nullptr);
		}
	}
	operands_.resize(count);
}


void SSAStatement::AddOperand(SSAValue* new_operand)
{
	CheckDisposal();

	new_operand->AddUse(this);
	operands_.push_back(new_operand);
}

SSAStatement::~SSAStatement()
{
#ifdef DEBUG_STATEMENTS
	_all_statements.erase(this);
#endif

	Parent = nullptr;

	for (auto i : operands_) {
		if (i != nullptr) {
			i->RemoveUse(this);
		}
	}
}

#ifdef DEBUG_STATEMENTS
std::unordered_set<SSAStatement*> SSAStatement::_all_statements;
#endif

void SSAStatement::CheckConsistency()
{
#ifdef DEBUG_STATEMENTS
	printf("Checking consistency...\n");
	for(auto i : _all_statements) {
		if(i->Parent == nullptr) {
			throw std::logic_error("Inconsistency detected - no parent");
		}
		if(!i->Parent->ContainsStatement(i)) {
			throw std::logic_error("Inconsistency detected - parent does not contain statement");
		}
	}
#endif
}

void SSAStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitStatement(*this);
}

const std::string& SSAStatement::GetISA() const
{
	return Parent->Parent->GetAction()->Context.ISA.ISAName;
}

bool SSAStatement::Resolve(DiagnosticContext &ctx)
{
	Resolved = true;
	return true;
}

bool SSAStatement::HasValue() const
{
	return GetType() != IRTypes::Void;
}

bool SSAStatement::HasSideEffects() const
{
	// assume by default that statements are important
	return true;
}

void SSAStatement::Unlink()
{
	for(unsigned i = 0; i < OperandCount(); ++i) {
		SetOperand(i, nullptr);
	}
}

const std::set<SSAStatement*> SSAStatement::GetUsedStatements()
{
	std::set<SSAStatement *> used_statements;

	for (unsigned int i = 0; i < OperandCount(); i++) {
		if (auto used_statement = dynamic_cast<SSAStatement *>(GetOperand(i))) {
			used_statements.insert(used_statement);
		}
	}

	return used_statements;
}
