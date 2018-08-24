/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"

#include "genC/ir/IRScope.h"
#include "genC/ir/IR.h"

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSABuilder::SSABuilder(SSAContext& context, const IRAction& action) : Context(context)
{
	const auto& signature = action.GetSignature();

	Target = new SSAFormAction(context, SSAActionPrototype(signature));
	Target->LinkAction(&action);
	Target->EntryBlock = new SSABlock(context, *Target);

	for (unsigned i = 0; i < signature.GetParams().size(); i++) {
		_param_symbol_map[signature.GetParams().at(i).GetName()] = Target->ParamSymbols.at(i);
	}

	curr_block = Target->EntryBlock;
}

void SSABuilder::AddInstruction(SSAStatement &insn)
{
	curr_block->AddStatement(insn);
}

void SSABuilder::AddBlock(SSABlock &block)
{
	assert(block.Parent == Target);
	Target->AddBlock(&block);
}

void SSABuilder::AddBlockAndChange(SSABlock &block)
{
	assert(block.Parent == Target);
	Target->AddBlock(&block);
	curr_block = &block;
}

void SSABuilder::ChangeBlock(SSABlock &block, bool EmitBranch)
{
	if (EmitBranch && curr_block->GetControlFlow() == NULL) {
		assert(false && "Unexpected change of block");
		new SSAJumpStatement(curr_block, block);
	}
	curr_block = &block;
}

void SSABuilder::EmitBranch(SSABlock &target, const IRStatement &statement)
{
	if(curr_block == nullptr) {
		throw std::logic_error("");
	}

	if(!curr_block->GetStatements().empty()) {
		if(curr_block->GetControlFlow() != nullptr) {
			throw std::logic_error("");
		}
	}

	auto js = new SSAJumpStatement(curr_block, target);
	js->SetDiag(statement.Diag());
}

SSABlock &SSABuilder::GetBlock()
{
	return *curr_block;
}

SSASymbol *SSABuilder::GetSymbol(const IRSymbol *sym)
{
	if (sym->SType == Symbol_Parameter) {
		if (_param_symbol_map.count(sym->GetLocalName()) == 0) throw std::logic_error("Unable to locate parameter symbol");
		return _param_symbol_map.at(sym->GetLocalName());
	}

	if (_symbol_map.count(sym) == 0) {
		auto new_symbol = new SSASymbol(Context, sym->GetLocalName(), sym->Type, sym->SType);
		Target->AddSymbol(new_symbol);
		_symbol_map.insert({sym, new_symbol});
	}

	return _symbol_map.at(sym);
}

SSASymbol *SSABuilder::InsertSymbol(const IRSymbol *sym, SSASymbol *referenceTo)
{
	assert(sym->Type != IRTypes::Void);

	IRType type = sym->Type;

	if (referenceTo != NULL) type.Reference = true;

	SSASymbol *new_sym = new SSASymbol(Context, sym->GetName(), type, sym->SType, referenceTo);
	Target->AddSymbol(new_sym);
	return new_sym;
}

SSASymbol *SSABuilder::InsertNewSymbol(const std::string name, const IRType &type, const SymbolType stype, SSASymbol *const referenceTo)
{
	IRType newType = type;

	if (referenceTo != NULL) newType.Reference = true;
	SSASymbol *new_sym = new SSASymbol(Context, name, newType, stype, referenceTo);
	Target->AddSymbol(new_sym);
	return new_sym;
}

SSASymbol *SSABuilder::GetTemporarySymbol(IRType type)
{
	SSASymbol *new_sym = new SSASymbol(Context, "temporary value", type, Symbol_Temporary, NULL);
	Target->AddSymbol(new_sym);
	return new_sym;
}

void SSABuilder::PushBreak(SSABlock &block)
{
	BreakStack.push(&block);
}

void SSABuilder::PushCont(SSABlock &block)
{
	ContStack.push(&block);
}

SSABlock &SSABuilder::PeekBreak()
{
	return *BreakStack.top();
}

SSABlock &SSABuilder::PeekCont()
{
	return *ContStack.top();
}

SSABlock &SSABuilder::PopBreak()
{
	SSABlock *blk = BreakStack.top();
	BreakStack.pop();
	return *blk;
}

SSABlock &SSABuilder::PopCont()
{
	SSABlock *blk = ContStack.top();
	ContStack.pop();
	return *blk;
}
