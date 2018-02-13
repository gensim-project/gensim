/*
 * IRRegisterExpression.cpp
 *
 *  Created on: 15 May 2015
 *      Author: harry
 */

#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRRegisterExpression.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSACastStatement.h"
#include "genC/ssa/statement/SSARegisterStatement.h"
#include "genC/ssa/SSABuilder.h"

using namespace gensim::genc;

/*
 * Register Slot Read
 */

IRRegisterSlotReadExpression::IRRegisterSlotReadExpression(IRScope &scope) : IRCallExpression(scope)
{

}

IRRegisterSlotReadExpression::~IRRegisterSlotReadExpression()
{

}

bool IRRegisterSlotReadExpression::Resolve(GenCContext &Context)
{
	bool success = true;
	success &= Args[0]->Resolve(Context);

	if(!success) return false;

	IRVariableExpression *c;
	if((c = dynamic_cast<IRVariableExpression*>(Args[0])) && (c->Symbol->SType == Symbol_Constant)) {
		slot_id = Context.GetConstant(c->Symbol->GetLocalName()).second;
	} else {
		Context.Diag().Error("Register slot reads must have a constant slot", Diag());
		success = false;
	}

	success &= ResolveParameter(Context, IRTypes::UInt32, Args[0], 1);

	Resolved = success;
	return success;
}

ssa::SSAStatement *IRRegisterSlotReadExpression::EmitSSAForm(ssa::SSABuilder &bldr) const
{
	auto stmt = ssa::SSARegisterStatement::CreateRead(&bldr.GetBlock(), slot_id);
	stmt->SetDiag(Diag());
	return stmt;
}

const IRType IRRegisterSlotReadExpression::EvaluateType()
{
	return GetRegisterType();
}

IRType IRRegisterSlotReadExpression::GetRegisterType() const
{
	IRType read_type = GetScope().GetContainingAction().Context.Arch.GetRegFile().GetSlotByIdx(slot_id).GetIRType();
	return read_type;
}


/*
 * Register Slot Write
 */

IRRegisterSlotWriteExpression::IRRegisterSlotWriteExpression(IRScope &scope) : IRCallExpression(scope)
{

}

IRRegisterSlotWriteExpression::~IRRegisterSlotWriteExpression()
{

}

bool IRRegisterSlotWriteExpression::Resolve(GenCContext &Context)
{
	if(Args.size() != 2) {
		Context.Diag().Error("Register write given incorrect number of arguments", Diag());
		return false;
	}

	bool success = true;
	success &= Args[0]->Resolve(Context);
	success &= Args[1]->Resolve(Context);

	if(!success) return false;

	IRVariableExpression *c;
	if((c = dynamic_cast<IRVariableExpression*>(Args[0])) && (c->Symbol->SType == Symbol_Constant)) {
		slot_id = Context.GetConstant(c->Symbol->GetLocalName()).second;
	} else {
		Context.Diag().Error("Register slot writes must have a constant slot", Diag());
		success = false;
	}


	success &= ResolveParameter(Context, IRTypes::UInt32, Args[0], 1);
	success &= ResolveParameter(Context, GetRegisterType(), Args[1], 2);

	value = Args[1];

	Resolved = success;
	return success;
}

ssa::SSAStatement *IRRegisterSlotWriteExpression::EmitSSAForm(ssa::SSABuilder &bldr) const
{
	ssa::SSAStatement *write_value = value->EmitSSAForm(bldr);

	if(write_value->GetType() != GetRegisterType()) {
		write_value = new ssa::SSACastStatement(&bldr.GetBlock(), GetRegisterType(), write_value);
		write_value->SetDiag(Diag());
	}

	auto stmt = ssa::SSARegisterStatement::CreateWrite(&bldr.GetBlock(), slot_id, write_value);
	stmt->SetDiag(Diag());

	return stmt;
}

const IRType IRRegisterSlotWriteExpression::EvaluateType()
{
	return IRTypes::Void;
}

IRType IRRegisterSlotWriteExpression::GetRegisterType() const
{
	IRType read_type = GetScope().GetContainingAction().Context.Arch.GetRegFile().GetSlotByIdx(slot_id).GetIRType();
	return read_type;
}


/*
 * Register Bank Read
 */

IRRegisterBankReadExpression::IRRegisterBankReadExpression(IRScope &scope) : IRCallExpression(scope)
{

}

IRRegisterBankReadExpression::~IRRegisterBankReadExpression()
{

}

bool IRRegisterBankReadExpression::Resolve(GenCContext &Context)
{
	bool success = true;

	if(Args.size() != 2) {
		Context.Diag().Error("Register bank reads must have two arguments", Diag());
		return false;
	}

	success &= Args[0]->Resolve(Context);
	success &= Args[1]->Resolve(Context);

	if(!success) return false;

	IRVariableExpression *c;
	if((c = dynamic_cast<IRVariableExpression*>(Args[0])) && (c->Symbol->SType == Symbol_Constant)) {
		bank_idx = Context.GetConstant(c->Symbol->GetLocalName()).second;
	} else {
		Context.Diag().Error("Register bank reads must have a constant slot", Diag());
		success = false;
	}

	const arch::RegBankViewDescriptor*rbv = &GetScope().GetContainingAction().Context.Arch.GetRegFile().GetBankByIdx(bank_idx);

	IRConstExpression *idx;
	if((idx = dynamic_cast<IRConstExpression*>(Args[1])) && (idx->Value.Int() >= rbv->GetRegisterCount())) {
		std::ostringstream str;
		str << "Read of register index " << idx->Value.Int() << " exceeds register count of bank view "<< rbv->ID;
		Context.Diag().Error(str.str(), Diag());
		success = false;
	}

	success &= ResolveParameter(Context, IRTypes::UInt32, Args[0], 1);
	success &= ResolveParameter(Context, IRTypes::UInt32, Args[1], 2);

	reg_slot_expression = Args[1];

	Resolved = success;
	return success;

}

ssa::SSAStatement *IRRegisterBankReadExpression::EmitSSAForm(ssa::SSABuilder &bldr) const
{
	ssa::SSAStatement *reg_slot_value = reg_slot_expression->EmitSSAForm(bldr);

	auto stmt = ssa::SSARegisterStatement::CreateBankedRead(&bldr.GetBlock(), bank_idx, reg_slot_value);
	stmt->SetDiag(Diag());

	return stmt;
}

const IRType IRRegisterBankReadExpression::EvaluateType()
{
	IRType read_type = GetScope().GetContainingAction().Context.Arch.GetRegFile().GetBankByIdx(bank_idx).GetRegisterIRType();
	return read_type;
}

IRType IRRegisterBankReadExpression::GetRegisterType() const
{
	IRType read_type = GetScope().GetContainingAction().Context.Arch.GetRegFile().GetBankByIdx(bank_idx).GetRegisterIRType();
	return read_type;
}

/*
 * Register Bank Write
 */

IRRegisterBankWriteExpression::IRRegisterBankWriteExpression(IRScope &scope) : IRCallExpression(scope)
{

}

IRRegisterBankWriteExpression::~IRRegisterBankWriteExpression()
{

}

bool IRRegisterBankWriteExpression::Resolve(GenCContext &Context)
{
	if(Args.size() != 3) {
		Context.Diag().Error("Register bank reads must have three arguments", Diag());
		return false;
	}

	bool success = true;
	success &= Args[0]->Resolve(Context);
	success &= Args[1]->Resolve(Context);
	success &= Args[2]->Resolve(Context);

	if(!success) return false;

	IRVariableExpression *c;
	if((c = dynamic_cast<IRVariableExpression*>(Args[0])) && (c->Symbol->SType == Symbol_Constant)) {
		bank_idx = Context.GetConstant(c->Symbol->GetLocalName()).second;
	} else {
		Context.Diag().Error("Register bank writes must have a constant slot", Diag());
		success = false;
	}

	const arch::RegBankViewDescriptor*rbv = &GetScope().GetContainingAction().Context.Arch.GetRegFile().GetBankByIdx(bank_idx);

	IRConstExpression *idx;
	if((idx = dynamic_cast<IRConstExpression*>(Args[1])) && (idx->Value.Int() >= rbv->GetRegisterCount())) {
		std::ostringstream str;
		str << "Read of register index " << idx->Value.Int() << " exceeds register count of bank view "<< rbv->ID;
		Context.Diag().Error(str.str(), Diag());
		success = false;
	}

	success &= ResolveParameter(Context, IRTypes::UInt32, Args[0], 1);
	success &= ResolveParameter(Context, IRTypes::UInt32, Args[1], 2);
	success &= ResolveParameter(Context, GetRegisterType(), Args[2], 3);

	reg_slot_expression = Args[1];
	value_expression = Args[2];

	Resolved = success;
	return success;

}

ssa::SSAStatement *IRRegisterBankWriteExpression::EmitSSAForm(ssa::SSABuilder &bldr) const
{
	ssa::SSAStatement *reg_slot_value = reg_slot_expression->EmitSSAForm(bldr);
	ssa::SSAStatement *write_value = value_expression->EmitSSAForm(bldr);

	if(write_value->GetType() != GetRegisterType()) {
		write_value = new ssa::SSACastStatement(&bldr.GetBlock(), GetRegisterType(), write_value);
		write_value->SetDiag(Diag());
	}

	auto stmt = ssa::SSARegisterStatement::CreateBankedWrite(&bldr.GetBlock(), bank_idx, reg_slot_value, write_value);
	stmt->SetDiag(Diag());

	return stmt;
}

const IRType IRRegisterBankWriteExpression::EvaluateType()
{
	return IRTypes::Void;
}

IRType IRRegisterBankWriteExpression::GetRegisterType() const
{
	IRType read_type = GetScope().GetContainingAction().Context.Arch.GetRegFile().GetBankByIdx(bank_idx).GetRegisterIRType();
	return read_type;
}
