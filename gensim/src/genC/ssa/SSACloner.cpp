/*
 * genC/ssa/SSAContext.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#include "genC/ssa/SSACloner.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAStatementVisitor.h"

#include "genC/ssa/statement/SSAStatements.h"

#include "define.h"

using namespace gensim::genc::ssa;

class SSAStatementCloner : public SSAStatementVisitor
{
public:
	SSAStatementCloner(SSAFormAction *new_action, SSACloneContext &ctx) : _block(nullptr), _target_action(new_action), _clone_context(ctx)
	{

	}

	void VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSABinaryArithmeticStatement(_block, _clone_context.get(stmt.LHS()), _clone_context.get(stmt.RHS()), stmt.Type));
	}

	void VisitCastStatement(SSACastStatement& stmt) override
	{
		auto cast_stmt = new SSACastStatement(_block, stmt.GetType(), _clone_context.get(stmt.Expr()), stmt.GetCastType());
		cast_stmt->SetOption(stmt.GetOption());
		_clone_context.add(&stmt, cast_stmt);
	}

	void VisitConstantStatement(SSAConstantStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAConstantStatement(_block, stmt.Constant, stmt.GetType()));
	}

	void VisitIfStatement(SSAIfStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAIfStatement(_block, _clone_context.get(stmt.Expr()), *_clone_context.get(stmt.TrueTarget()), *_clone_context.get(stmt.FalseTarget())));
	}

	void VisitJumpStatement(SSAJumpStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAJumpStatement(_block, *_clone_context.get(stmt.Target())));
	}

	void VisitCallStatement(SSACallStatement& stmt) override
	{
		std::vector<SSAValue *> mapped_values;

		for(unsigned i = 0; i < stmt.ArgCount(); ++i) {
			SSAValue *arg = stmt.Arg(i);

			SSAValue *mapped_arg = _clone_context.getvalue(arg);
			mapped_values.push_back(mapped_arg);
		}

		// need to map the callee!!??
		_clone_context.add(&stmt, new SSACallStatement(_block, stmt.Target(), mapped_values));

	}

	void VisitMemoryReadStatement(SSAMemoryReadStatement& stmt) override
	{
		_clone_context.add(&stmt, &SSAMemoryReadStatement::CreateRead(_block, _clone_context.get(stmt.Addr()), _clone_context.get(stmt.Target()), stmt.Width, stmt.Signed, stmt.GetInterface()));
	}

	void VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt) override
	{
		_clone_context.add(&stmt, &SSAMemoryWriteStatement::CreateWrite(_block, _clone_context.get(stmt.Addr()), _clone_context.get(stmt.Value()), stmt.Width, stmt.User));
	}

	void VisitRegisterStatement(SSARegisterStatement& stmt) override
	{
		if(stmt.IsBanked) {
			if(stmt.IsRead) {
				_clone_context.add(&stmt, SSARegisterStatement::CreateBankedRead(_block, stmt.Bank, _clone_context.get(stmt.RegNum())));
			} else {
				_clone_context.add(&stmt, SSARegisterStatement::CreateBankedWrite(_block, stmt.Bank, _clone_context.get(stmt.RegNum()), _clone_context.get(stmt.Value())));
			}
		} else {
			if(stmt.IsRead) {
				_clone_context.add(&stmt, SSARegisterStatement::CreateRead(_block, stmt.Bank));
			} else {
				_clone_context.add(&stmt, SSARegisterStatement::CreateWrite(_block, stmt.Bank, _clone_context.get(stmt.Value())));
			}

		}
	}

	void VisitRaiseStatement(SSARaiseStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSARaiseStatement(_block));
	}

	void VisitReturnStatement(SSAReturnStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAReturnStatement(_block, _clone_context.get(stmt.Value())));
	}

	void VisitStatement(SSAStatement& stmt) override
	{
		UNIMPLEMENTED;
	}

	void VisitVariableReadStatement(SSAVariableReadStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAVariableReadStatement(_block, _clone_context.get(stmt.Target())));
	}

	void VisitVariableWriteStatement(SSAVariableWriteStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAVariableWriteStatement(_block, _clone_context.get(stmt.Target()), _clone_context.get(stmt.Expr())));
	}

	void VisitControlFlowStatement(SSAControlFlowStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitDeviceReadStatement(SSADeviceReadStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSADeviceReadStatement(_block, _clone_context.get(stmt.Device()), _clone_context.get(stmt.Address()), _clone_context.get(stmt.Target())));
	}
	void VisitPhiStatement(SSAPhiStatement& stmt) override
	{
		auto newstmt = new SSAPhiStatement(_block, stmt.GetType());

		_clone_context.add(&stmt, newstmt);
	}

	void VisitIntrinsicStatement(SSAIntrinsicStatement &stmt) override
	{
		auto newstmt = new SSAIntrinsicStatement(_block, stmt.Type);

		for(unsigned i = 0; i < stmt.ArgCount(); ++i) {
			newstmt->AddArg(_clone_context.get(stmt.Args(i)));
		}

		_clone_context.add(&stmt, newstmt);

	}
	void VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt) override
	{
		auto newstmt = new SSAReadStructMemberStatement(_block, _clone_context.get(stmt.Target()), stmt.MemberName, stmt.Index);
		_clone_context.add(&stmt, newstmt);
	}

	void VisitSelectStatement(SSASelectStatement &stmt) override
	{
		_clone_context.add(&stmt, new SSASelectStatement(_block, _clone_context.get(stmt.Cond()), _clone_context.get(stmt.TrueVal()), _clone_context.get(stmt.FalseVal())));
	}

	void VisitSwitchStatement(SSASwitchStatement &stmt) override
	{
		SSASwitchStatement *newstmt = new SSASwitchStatement(_block, _clone_context.get(stmt.Expr()), _clone_context.get(stmt.Default()));

		for(auto i : stmt.GetValues()) {
			newstmt->AddValue(_clone_context.get(i.first), _clone_context.get(i.second));
		}

		_clone_context.add(&stmt, newstmt);
	}

	void VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAUnaryArithmeticStatement(_block, _clone_context.get(stmt.Expr()), stmt.Type));
	}
	void VisitVariableKillStatement(SSAVariableKillStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAVectorExtractElementStatement(_block, _clone_context.get(stmt.Base()), _clone_context.get(stmt.Index())));
	}
	void VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSAVectorInsertElementStatement(_block, _clone_context.get(stmt.Base()), _clone_context.get(stmt.Index()), _clone_context.get(stmt.Value())));
	}

	void VisitBitDepositStatement(SSABitDepositStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSABitDepositStatement(_block, _clone_context.get(stmt.Expr()), _clone_context.get(stmt.BitFrom()), _clone_context.get(stmt.BitTo()), _clone_context.get(stmt.Value())));
	}

	void VisitBitExtractStatement(SSABitExtractStatement& stmt) override
	{
		_clone_context.add(&stmt, new SSABitExtractStatement(_block, _clone_context.get(stmt.Expr()), _clone_context.get(stmt.BitFrom()), _clone_context.get(stmt.BitTo())));
	}

	SSABlock *MapBlock(SSABlock *source_block)
	{
		return _clone_context.get(source_block);
	}

	void SetBlock(SSABlock *block)
	{
		_block = block;
	}

private:


	SSABlock *_block;
	SSAStatement *_statement;
	SSAFormAction *_target_action;

	SSACloneContext &_clone_context;
};

SSACloneContext::SSACloneContext(SSAFormAction* target_action) : _target_action(target_action)
{
}

void SSACloneContext::add(const SSAStatement* source, SSAStatement* cloned)
{
	if (Statements().count(source)) {
		throw std::logic_error("");
	}

	CloneMetadata(source, cloned);
	Statements()[source] = cloned;
}

void SSACloneContext::add(const SSABlock* source, SSABlock* cloned)
{
	if (Blocks().count(source)) {
		throw std::logic_error("");
	}

	CloneMetadata(source, cloned);
	Blocks()[source] = cloned;
}

SSAStatement* SSACloneContext::get(const SSAStatement* source)
{
	if (source == nullptr) {
		return nullptr;
	}
	return Statements().at(source);
}

SSABlock* SSACloneContext::get(const SSABlock* source)
{
	if (Blocks().count(source) == 0) {
		SSABlock *new_block = new SSABlock(_target_action->GetContext(), *_target_action);
		Blocks()[source] = new_block;
	}
	return Blocks().at(source);
}

SSASymbol* SSACloneContext::get(const SSASymbol* source)
{
	if (Symbols().count(source) == 0) {
		SSASymbol *new_sym = new SSASymbol(_target_action->GetContext(), source->GetPrettyName(), source->GetType(), source->SType);
		_target_action->AddSymbol(new_sym);
		CloneMetadata(source, new_sym);
		Symbols()[source] = new_sym;
	}

	return Symbols().at(source);
}

SSAValue* SSACloneContext::getvalue(const SSAValue* source)
{
	if (auto statement = dynamic_cast<const SSAStatement*> (source)) {
		return get(statement);
	}
	if (auto block = dynamic_cast<const SSABlock*> (source)) {
		return get(block);
	}
	if (auto symbol = dynamic_cast<const SSASymbol*> (source)) {
		return get(symbol);
	}

	throw std::logic_error("Unknown value type");
}

void SSACloneContext::CloneMetadata(const SSAValue* source, SSAValue* cloned)
{
	for (const auto& md : source->GetMetadata()) {
		if (auto dnmd = dynamic_cast<DiagnosticNodeMetadata *>(md)) {
			cloned->AddMetadata(new DiagnosticNodeMetadata(dnmd->GetDiagNode()));
		} else {
			throw std::logic_error("Unsupported metadata node type for cloning");
		}
	}
}

SSABlock* SSACloner::Clone(SSABlock* block, SSAFormAction *new_parent, SSACloneContext &ctx)
{
	SSABlock *new_block = ctx.get(block);

	SSAStatementCloner cloner (new_parent, ctx);
	cloner.SetBlock(new_block);
	for (auto i : block->GetStatements()) {
		i->Accept(cloner);
	}

	return new_block;
}

SSAFormAction* SSACloner::Clone(const SSAFormAction* source)
{
	SSAFormAction *new_action = new SSAFormAction(source->GetContext(), source->GetPrototype());
	new_action->Arch = source->Arch;
	new_action->Isa = source->Isa;

	SSACloneContext ctx (new_action);
	SSAStatementCloner cloner (new_action, ctx);

	for(auto i : source->Blocks) {
		cloner.MapBlock(i);
	}
	new_action->EntryBlock = cloner.MapBlock(source->EntryBlock);

	for(auto i : source->Blocks) {
		cloner.SetBlock(cloner.MapBlock(i));
		for(auto j : i->GetStatements()) {
			j->Accept(cloner);
		}
	}

	// now, need to go back and fix up phi nodes
	for(auto i : ctx.Statements()) {
		if(dynamic_cast<const SSAPhiStatement*>(i.first)) {
			const SSAPhiStatement *source = (const SSAPhiStatement*)i.first;
			SSAPhiStatement *cloned = (SSAPhiStatement*)i.second;

			for(auto member : source->Get()) {
				cloned->Add(ctx.get((SSAStatement*)member));
			}

			GASSERT(!cloned->Get().empty());
		}
	}

	return new_action;
}
