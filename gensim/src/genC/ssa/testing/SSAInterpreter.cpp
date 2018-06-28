/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "genC/ssa/testing/SSAInterpreter.h"
#include "genC/ssa/testing/SSAInterpreterStatementVisitor.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "arch/ArchDescription.h"

using namespace gensim::genc;
using namespace gensim::genc::ssa::testing;

void VMActionState::SetStatementValue(SSAStatement* stmt, IRConstant value)
{
	GASSERT(value.Type() != IRConstant::Type_Invalid);
	Statements()[stmt] = {timestamp_, IRType::Cast(value, stmt->GetType(), stmt->GetType())};

	timestamp_++;
}

void VMActionState::SetSymbolValue(SSASymbol* symbol, IRConstant value)
{
	GASSERT(value.Type() != IRConstant::Type_Invalid);
	symbols_[symbol] = IRType::Cast(value, symbol->GetType(), symbol->GetType());
}

uint64_t VMActionState::GetStatementTimestamp(SSAStatement* stmt) const
{
	return Statements().at(stmt).first;
}


const gensim::genc::IRConstant& VMActionState::GetStatementValue(SSAStatement* stmt)
{
	return Statements().at(stmt).second;
}

IRConstant GetDefaultValue(const IRType &type)
{
	GASSERT(!type.IsStruct());
	if(type.VectorWidth > 1) {
		return IRConstant::Vector(type.VectorWidth, GetDefaultValue(type.GetElementType()));
	} else {
		switch(type.BaseType.PlainOldDataType) {
			case IRPlainOldDataType::FLOAT:
				return IRConstant::Float(0);
			case IRPlainOldDataType::DOUBLE:
				return IRConstant::Double(0);
			default:
				return IRConstant::Integer(0);
		}
	}
}

const gensim::genc::IRConstant& VMActionState::GetSymbolValue(SSASymbol* symbol)
{
	if(!symbols_.count(symbol)) {
		SetSymbolValue(symbol, GetDefaultValue(symbol->GetType()));
	}

	return symbols_.at(symbol);
}


void VMActionState::Reset()
{
	symbols_.clear();
	statements_.clear();
}

SSAInterpreter::SSAInterpreter(const arch::ArchDescription *arch, machine_state_t &target_state) : arch_(arch), state_(target_state), trace_(false)
{

}


void SSAInterpreter::Reset()
{
	state_.Reset();
}

ActionResult SSAInterpreter::ExecuteAction(const SSAFormAction* action, const std::vector<IRConstant> &param_values)
{
	BlockResult result;
	result.NextBlock = action->EntryBlock;
	result.Result = Interpret_Normal;

	VMActionState actionstate;
	if(param_values.size() != action->ParamSymbols.size()) {
		throw std::logic_error("");
	}
	for(unsigned i = 0; i < param_values.size(); ++i) {
		actionstate.SetSymbolValue(action->ParamSymbols[i], param_values[i]);
	}

	while(result.NextBlock != nullptr && result.Result == Interpret_Normal) {
		result = ExecuteBlock(result.NextBlock, actionstate);
	}

	ActionResult aresult;
	aresult.Result = result.Result;
	if(action->GetPrototype().ReturnType() != IRTypes::Void) {
		aresult.ReturnValue = actionstate.GetReturnValue();
	}

	auto &parameter_types = action->GetPrototype().ParameterTypes();
	for(unsigned i = 0; i < parameter_types.size(); ++i) {
		if(parameter_types[i].Reference) {
			aresult.ReferenceParameterValues[i] = actionstate.GetSymbolValue(action->ParamSymbols[i]);
		}
	}

	return aresult;
}

SSAInterpreter::BlockResult SSAInterpreter::ExecuteBlock(SSABlock* block, VMActionState& vmstate)
{
	// execute each statement
	SSAInterpreter::BlockResult result;
	result.NextBlock = nullptr;
	result.Result = Interpret_Normal;

	SSAInterpreterStatementVisitor visitor (vmstate, state_, arch_);
	for(auto statement : block->GetStatements()) {
		if(trace_) {
			statement->Dump();
		}

		statement->Accept(visitor);

		if(vmstate.GetResult() != Interpret_Normal) {
			result.Result = vmstate.GetResult();
			return result;
		}
	}

	result.Result = Interpret_Normal;
	result.NextBlock = vmstate.GetNextBlock();
	return result;
}
