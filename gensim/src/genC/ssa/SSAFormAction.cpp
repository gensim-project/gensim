/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"

#include "genC/Parser.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSACloner.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSACallStatement.h"
#include "genC/ssa/statement/SSACastStatement.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAControlFlowStatement.h"
#include "genC/ssa/statement/SSAReturnStatement.h"
#include "genC/ssa/statement/SSARaiseStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSASelectStatement.h"
#include "genC/ssa/printers/SSAActionPrinter.h"
#include "genC/ssa/SSASymbol.h"
#include "ComponentManager.h"
#include "genC/ssa/passes/SSAPass.h"

#include <typeindex>

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSAActionBase::SSAActionBase(SSAContext& context, const SSAActionPrototype& prototype) : SSAValue(context, context.GetValueNamespace()), prototype_(prototype), _type(IRTypes::Function)
{

}

SSAActionBase::~SSAActionBase()
{

}

void SSAActionBase::Destroy()
{

}


SSAFormAction::SSAFormAction(SSAContext& context, const SSAActionPrototype &prototype)
	: SSAActionBase(context, prototype),
	  EntryBlock(nullptr),
	  action_(nullptr),
	  Arch(nullptr)
{
	for (const auto param : GetPrototype().GetIRSignature().GetParams()) {
		std::string paramname = "_P_" + param.GetName();
		SSASymbol *sym = new SSASymbol(context, "parameter " + param.GetName(), param.GetType(), Symbol_Parameter);
		AddSymbol(sym);
		ParamSymbols.push_back(sym);
	}
}

SSAFormAction::StatementList SSAFormAction::GetStatements(std::function<bool(SSAStatement*)> fn) const
{
	StatementList output;
	for(auto block : GetBlocks()) {
		for(auto stmt : block->GetStatements()) {
			if(fn(stmt)) {
				output.push_back(stmt);
			}
		}
	}
	return output;
}


bool SSAFormAction::ContainsBlock(const SSABlock *block) const
{
	for (auto i : GetBlocks()) {
		if (i == block) return true;
	}
	return false;
}

void SSAFormAction::RemoveBlock(SSABlock* block)
{
	if(block == EntryBlock) {
		throw std::logic_error("Attempted to remove the entry block from an action");
	}
	if (!ContainsBlock(block)) {
		throw std::logic_error("Tried to remove a block which wasn't contained");
	}

	for(auto block : blocks_ ) {
		block->ClearID();
	}

	for (auto i = blocks_.begin(); i != blocks_.end(); ++i) {
		if (*i == block) {
			blocks_.erase(i);
			break;
		}
	}

	block->Parent = nullptr;
	block->RemoveUse(this);
	RemoveUse(block);
}

void SSAFormAction::AddSymbol(SSASymbol* new_symbol)
{
	_symbols.insert(new_symbol);
	new_symbol->AddUse(this);
}

void SSAFormAction::RemoveSymbol(SSASymbol* symbol)
{
	symbol->RemoveUse(this);
	_symbols.erase(symbol);

	for(auto b : GetBlocks()) {
		b->ClearFixedness();
	}
}


const SSASymbol* SSAFormAction::GetSymbol(const std::string symname) const
{
	for (auto sym : _symbols) {
		if (sym->GetName() == symname) {
			return sym;
		}
	}

	return nullptr;
}

const SSASymbol* SSAFormAction::GetSymbol(const char *symname) const
{
	return GetSymbol((std::string)symname);
}

SSASymbol* SSAFormAction::GetSymbol(const std::string symname)
{
	for (auto sym : _symbols) {
		if (sym->GetName() == symname) {
			return sym;
		}
	}

	return nullptr;
}

SSASymbol* SSAFormAction::GetSymbol(const char *symname)
{
	return GetSymbol((std::string)symname);
}

std::string SSAFormAction::GetName() const
{
	return "a_" + std::to_string(GetValueName()) + "_" + GetPrototype().GetIRSignature().GetName();
}

SSAFormAction* SSAFormAction::Clone() const
{
	SSACloner cloner;
	return cloner.Clone(this);
}

void SSAFormAction::AddBlock(SSABlock* block)
{
	if (block == nullptr) {
		throw std::logic_error("Tried to add a null block");
	}
	if (ContainsBlock(block)) {
		throw std::logic_error("Tried to add a block which was already contained");
	}
	if (block->Parent != nullptr) {
		throw std::logic_error("Tried to add a block which already had a parent");
	}

	blocks_.push_back(block);
	block->Parent = this;
	block->AddUse(this);
	block->ClearID();
	AddUse(block);

}

void SSAFormAction::LinkAction(const IRAction* action)
{
	action_ = action;
	Arch = &action_->Context.Arch;
}

bool SSAFormAction::DoFixednessAnalysis()
{
	// loop through all blocks, resetting their fixedness information
	for (auto block : GetBlocks()) {
		block->ClearFixedness();
	}

	// fprintf(stderr, "Fixedness analysis for %s\n", Action.Name.c_str());

	// also mark all variable uses as fixed
	for (SymbolTableConstIterator ci = Symbols().begin(); ci != Symbols().end(); ++ci) {
		for (auto use : (*ci)->Uses) {
			use->Const = true;
		}
	}

	std::list<SSABlock *> wl;
	wl.push_back(EntryBlock);
	while (wl.size() > 0) {
		SSABlock *block = wl.front();
		wl.pop_front();
		bool done = block->InitiateFixednessAnalysis();
		// fprintf(stderr, "Analysis of %s %s (%u successors)\n", block->GetName().c_str(), done ? "complete" : "incomplete", block->GetSuccessors().size());
		if (!done) {
			auto successors = block->GetSuccessors();
			wl.insert(wl.end(), successors.begin(), successors.end());
		}
	}

	return true;
}

void SSAFormAction::Unlink()
{
	// remove any references to other actions (via calls)
	for(auto block : GetBlocks()) {
		for(auto stmt : block->GetStatements()) {
			if(SSACallStatement *call = dynamic_cast<SSACallStatement*>(stmt)) {
				call->SetTarget(nullptr);
			}
		}
	}
}

void SSAFormAction::Destroy()
{
	// unlink control flow
	EntryBlock = nullptr;
	for (auto i : GetBlocks()) {
		for(auto stmt : i->GetStatements()) {
			stmt->Unlink();
		}
	}

	while (!blocks_.empty()) {
		auto block = blocks_.back();
		RemoveBlock(block);
		block->Destroy();
		delete block;
	}

	while(!Symbols().empty()) {
		auto symbol = *Symbols().begin();
		RemoveSymbol(symbol);
		symbol->Dispose();
		delete symbol;
	}

	Dispose();
}

bool SSAFormAction::Resolve(DiagnosticContext &ctx)
{
	bool success = true;

	bool returns = DoCheckReturn();
	if (!returns) {
		if (GetPrototype().GetIRSignature().GetType() != IRTypes::Void) {
			ctx.Error("Not all paths in " + GetPrototype().GetIRSignature().GetName() + " return a value", GetDiag());
		} else {
			ctx.Error("Not all paths in " + GetPrototype().GetIRSignature().GetName() + " return", GetDiag());
		}
		return false;
	}

	if (success) {
		for (auto block : GetBlocks()) {
			success &= block->Resolve(ctx);
		}
	}

	return success;
}

std::list<SSAVariableReadStatement *> SSAFormAction::GetDominatedReads(const SSAVariableKillStatement * const stmt) const
{

	const SSABlock *startBlock = stmt->Parent;
	std::list<SSABlock *>::const_iterator b_i = GetBlocks().begin();
	while (*b_i != startBlock) b_i++;

	std::list<SSAVariableReadStatement *> DominatedReads;

	// Start by adding any dominated reads in the starting block (and terminate if there are any
	// writes
	auto s_i = startBlock->GetStatements().begin();
	while (*s_i != stmt) s_i++;

	// skip the instruction that we are testing
	s_i++;

	while (s_i != startBlock->GetStatements().end()) {
		if (SSAVariableReadStatement * read = dynamic_cast<SSAVariableReadStatement *> (*s_i)) {
			if (read->Target() == stmt->Target()) {
				DominatedReads.push_back(read);
			}
		}			// terminate early if we come across any writes to the same symbol in the same block
		else if (SSAVariableKillStatement * write = dynamic_cast<SSAVariableKillStatement *> (*s_i)) {
			if (write->Target() == stmt->Target()) {
				return DominatedReads;
			}
		}
		s_i++;
	}

	std::list<const SSABlock *> blockQueue;
	std::set<const SSABlock *> checkedBlocks;

	// loop through all reachable blocks
	auto successors = startBlock->GetSuccessors();
	blockQueue.insert(blockQueue.begin(), successors.begin(), successors.end());
	while (blockQueue.size() > 0) {
		const SSABlock *block = blockQueue.front();
		blockQueue.pop_front();

		if (checkedBlocks.find(block) != checkedBlocks.end()) continue;

		// in each block, look for reads and writes to the same symbol
		bool SearchSuccessors = true;
		for (s_i = block->GetStatements().begin(); s_i != block->GetStatements().end(); ++s_i) {
			if (SSAVariableReadStatement * read = dynamic_cast<SSAVariableReadStatement *> (*s_i)) {
				if (read->Target() == stmt->Target()) {
					//                fprintf(stderr, "%s dominates %s\n", stmt->GetName().c_str(), read->GetName().c_str());
					DominatedReads.push_back(read);
				}
			}				// terminate early if we come across any writes to the same symbol in the same block
			else if (SSAVariableKillStatement * write = dynamic_cast<SSAVariableKillStatement *> (*s_i)) {
				if (write->Target() == stmt->Target()) {
					//         fprintf(stderr, "%s overriden by %s\n", stmt->GetName().c_str(), write->GetName().c_str());
					SearchSuccessors = false;
					break;
				}
			}
		}

		if (SearchSuccessors) {
			auto successors = block->GetSuccessors();
			blockQueue.insert(blockQueue.begin(), successors.begin(), successors.end());
		}
		checkedBlocks.insert(block);
	}
	return DominatedReads;
}

bool SSAFormAction::HasDynamicDominatedReads(const SSAVariableKillStatement *stmt) const
{
	std::list<SSAVariableReadStatement *> dominated_reads = GetDominatedReads(stmt);
	for (std::list<SSAVariableReadStatement *>::const_iterator ci = dominated_reads.begin(); ci != dominated_reads.end(); ++ci) {
		const SSAVariableReadStatement &read = **ci;
		if (!read.IsFixed() || read.Parent->IsFixed() != BLOCK_ALWAYS_CONST) return true;
	}
	return false;
}

bool SimplifiesToConstant(SSAStatement *stmt, uint64_t &out)
{
	if (SSAConstantStatement * constant = dynamic_cast<SSAConstantStatement*> (stmt)) {
		out = constant->Constant.Int();
		return true;
	}
	if (SSACastStatement * cast = dynamic_cast<SSACastStatement*> (stmt)) {
		uint64_t cast_constant;
		if (SimplifiesToConstant(cast->Expr(), cast_constant)) {
			//TODO: Actually cast the value here
			return cast_constant;
		}
	}
	return false;
}

bool SSAFormAction::DoCheckReturn() const
{
	std::list<const SSABlock *> work_list = {EntryBlock};
	std::set<const SSABlock*> examined_blocks;

	while (!work_list.empty()) {
		const SSABlock *block = work_list.front();
		work_list.pop_front();

		examined_blocks.insert(block);

		const SSAStatement *terminator = block->GetControlFlow();

		// Control flow statement should definitely not be null
		if (terminator == NULL) {
			return false;
		}

		// If this block has no successors, then the control flow statement should be a return or a raise.
		if (block->GetSuccessors().empty()) {
			const SSARaiseStatement *raise_stmt = dynamic_cast<const SSARaiseStatement*> (terminator);
			const SSAReturnStatement *return_stmt = dynamic_cast<const SSAReturnStatement*> (terminator);

			if (return_stmt == nullptr && raise_stmt == nullptr) {
				return false;
			}

			// check to see if we should return a value
			if(GetPrototype().GetIRSignature().HasReturnValue() && return_stmt != nullptr && return_stmt->Value() == nullptr) {
				return false;
			}
		}

		for (const auto *next : block->GetSuccessors()) {
			if (!examined_blocks.count(next)) {
				work_list.push_back(next);
			}
		}
	}

	return true;
}

std::string SSAFormAction::ToString() const
{
	printers::SSAActionPrinter action_printer(*this);
	return action_printer.ToString();
}
