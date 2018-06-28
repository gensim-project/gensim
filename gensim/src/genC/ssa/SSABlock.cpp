/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAControlFlowStatement.h"
#include "genC/ssa/statement/SSAIfStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/statement/SSAMemoryReadStatement.h"
#include "genC/ssa/statement/SSASwitchStatement.h"
#include "genC/ssa/statement/SSAReturnStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/printers/SSABlockPrinter.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/analysis/SSADominance.h"

#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"

#include "genC/Parser.h"

#include <queue>
#include <unordered_set>
#include <typeinfo>
#include <typeindex>

#define DEBUG 0
#define DPRINTF(...) do { if(DEBUG) printf(__VA_ARGS__); } while(0)

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

void SSABlock::AddStatement(SSAStatement &stmt)
{
	CheckDisposal();
	assert(&stmt);
	assert(stmt.Parent == this);
	Statements.push_back(&stmt);
	stmt.AddUse(this);
}

void SSABlock::AddStatement(SSAStatement &stmt, SSAStatement &before)
{
	CheckDisposal();
	assert(&stmt);
	assert(!stmt.IsDisposed());
	assert(stmt.Parent == this);
	assert(before.Parent == this);
	auto before_i = Statements.begin();
	while ((*before_i) != &before) {
		if (before_i == Statements.end()) {
			throw std::logic_error("");
		}
		before_i++;
	}

	stmt.AddUse(this);
	Statements.insert(before_i, &stmt);
}

void SSABlock::RemoveStatement(SSAStatement &stmt)
{
	CheckDisposal();
	assert(&stmt);
	assert(!stmt.IsDisposed());
	assert(stmt.Parent == this);

	for (auto i = Statements.begin(); i != Statements.end(); ++i) {
		if (*i == &stmt) {
			Statements.erase(i);
			break;
		}
	}

	stmt.RemoveUse(this);
}

void SSABlock::AddPredecessor(SSABlock &stmt)
{
	CheckDisposal();
	assert(&stmt);
	assert(stmt.Parent == Parent);
}

void SSABlock::RemovePredecessor(SSABlock &stmt)
{
	assert(stmt.Parent == Parent);
	assert(&stmt);
}

const SSABlock::BlockList SSABlock::GetPredecessors() const
{
	CheckDisposal();

	if (Parent == nullptr) return {};

	BlockList predecessors;
	for (std::list<SSABlock *>::const_iterator ci = Parent->Blocks.begin(); ci != Parent->Blocks.end(); ++ci) {
		SSABlock::BlockList succs = (*ci)->GetSuccessors();
		for (auto si = succs.begin(); si != succs.end(); ++si) {
			if (*si == this) {
				predecessors.push_back(*ci);
			}
		}
	}

	return predecessors;
}

SSABlock::BlockList SSABlock::GetSuccessors()
{
	CheckDisposal();

	if (Statements.empty()) return {};

	SSAControlFlowStatement *s = GetControlFlow();
	if(s == nullptr) {
		throw std::logic_error("Cannot get successors of a block with no control flow statement");
	}
	return s->GetTargets();
}

SSABlock::BlockConstList SSABlock::GetSuccessors() const
{
	CheckDisposal();

	if (Statements.empty()) return {};

	const SSAControlFlowStatement *s = GetControlFlow();
	if(s == nullptr) {
		throw std::logic_error("Cannot get successors of a block with no control flow statement");
	}
	return s->GetTargets();
}

const SSABlock::StatementList &SSABlock::GetStatements() const
{
	CheckDisposal();
	return Statements;
}

const SSAVariableWriteStatement *SSABlock::GetLastWriteTo(const SSASymbol *sym) const
{
	return GetLastWriteTo(sym, Statements.rbegin());
}

const SSAVariableWriteStatement *SSABlock::GetLastWriteTo(const SSASymbol *sym, const SSAStatement *startpos) const
{
	assert(startpos->Parent == this);
	StatementListConstReverseIterator i = Statements.rbegin();
	while (*i != startpos) ++i;
	return GetLastWriteTo(sym, i);
}

const SSAVariableWriteStatement *SSABlock::GetLastWriteTo(const SSASymbol *symbol, StatementListConstReverseIterator startpos) const
{
	while (startpos != Statements.rend()) {
		const SSAStatement *stmt = *startpos;

		if (const SSAVariableWriteStatement * write = dynamic_cast<const SSAVariableWriteStatement *> (stmt)) {
			if (write->Target() == symbol) return write;
		}

		startpos++;
	}
	return NULL;
}

SSABlock::SSABlock(SSAContext& context, SSAFormAction &parent) : SSAValue(context, context.GetValueNamespace()), Parent(nullptr), _constness(BLOCK_ALWAYS_CONST), _type(IRTypes::Block)
{
	parent.AddBlock(this);
}

SSABlock::SSABlock(SSABuilder &bldr) : SSAValue(bldr.Context, bldr.Context.GetValueNamespace()), _type(IRTypes::Block), Parent(nullptr), _constness(BLOCK_ALWAYS_CONST)
{
	bldr.Target->AddBlock(this);
}

SSABlock::~SSABlock()
{
	assert(IsDisposed());
}

std::string SSABlock::GetName() const
{
	CheckDisposal();

	if (Parent == nullptr) return "(detached parent)";

	int Number = 0;
	// this is a horrible way of getting the block name every time but means we don't have to
	// manually keep track of block naming/numbering
	for (std::list<SSABlock *>::const_iterator ci = Parent->Blocks.begin(); ci != Parent->Blocks.end(); ++ci) {
		if (*ci == this) break;
		Number++;
	}

	return "b_" + std::to_string(Number);
}

uint32_t SSABlock::GetID() const
{
	CheckDisposal();
	uint32_t id = 0;
	for (auto block : Parent->Blocks) {
		if (block == this) break;
		id++;
	}

	return id;
}

const std::set<SSABlock *> SSABlock::GetAllPredecessors() const
{
	CheckDisposal();
	std::set<SSABlock *> PredecessorSet;
	std::queue<SSABlock *> PredecessorQueue;

	auto preds = GetPredecessors();
	for (auto ci = preds.begin(); ci != preds.end(); ++ci) {
		PredecessorQueue.push(*ci);
	}

	while (!PredecessorQueue.empty()) {
		SSABlock *pred = PredecessorQueue.front();
		PredecessorQueue.pop();
		PredecessorSet.insert(pred);

		preds = pred->GetPredecessors();
		for (auto ci = preds.begin(); ci != preds.end(); ++ci) {
			if (PredecessorSet.find(*ci) == PredecessorSet.end()) PredecessorQueue.push(*ci);
		}
	}
	return PredecessorSet;
}

SSABlockConstness SSABlock::IsFixed() const
{
	CheckDisposal();
	return _constness;
}

bool SSABlock::Resolve(DiagnosticContext &ctx)
{
	CheckDisposal();
	bool success = true;
	//DoInsertTemporaries();
	Cleanup();
	for (auto i = Statements.begin(); i != Statements.end(); ++i) {
		success &= (*i)->Resolve(ctx);
	}

	assert(GetStatements().empty() || GetControlFlow());

	return success;
}

static bool Fixedness_Debug_Enabled = false;
void EnableFixednessDebug()
{
	Fixedness_Debug_Enabled = true;
}

#define DEBUG_FIXEDNESS(x) do { if(Fixedness_Debug_Enabled) { x } } while(0)

bool SSABlock::InitiateFixednessAnalysis()
{
	DEBUG_FIXEDNESS(fprintf(stderr, "CF analysis starting for %s of %s\n", GetName().c_str(), Parent->GetPrototype().GetIRSignature().GetName().c_str()););
	CheckDisposal();
	// Now, if we write a non const value to a variable in the ConstOut set, we remove it from the ConstOut set.
	// We also remove variables which are freed and add variables which are allocated. Allocations must happen
	// at the top of a block so we can do this in program order

	// First we need to update whether or not this block is const
	SSABlockConstness prev_constness = _constness;
	// loop through each of our predecessors and examine their control flow statement
	// if they are all const, update our constness to always_const.
	// else, update out constness to never_const

	_constness = BLOCK_ALWAYS_CONST;

	auto preds = GetPredecessors();
	for (auto b_i = preds.begin(); b_i != preds.end(); ++b_i) {
		SSABlock *pred = *b_i;

		if ((!pred->GetControlFlow()->IsFixed()) || pred->IsFixed() == BLOCK_NEVER_CONST) {
			_constness = BLOCK_NEVER_CONST;
			break;
		}
	}

	DEBUG_FIXEDNESS(fprintf(stderr, "CF analysis done: %u -> %u\n", prev_constness, _constness););

	// remove any symbols not in the ConstOut sets of our predecessors
	for (auto ci = preds.begin(); ci != preds.end(); ++ci) {
		std::vector<SSASymbol *> symbol_union(DynamicIn.size() + (*ci)->DynamicOut.size());
		std::vector<SSASymbol *>::iterator i = std::set_union(DynamicIn.begin(), DynamicIn.end(), (*ci)->DynamicOut.begin(), (*ci)->DynamicOut.end(), symbol_union.begin());

		//DEBUG_FIXEDNESS(fprintf(stderr, "In pred %s, %u symbols are dynamic (intersection yields %u)\n", (*ci)->GetName().c_str(), (*ci)->DynamicOut.size(), std::distance(intersection.begin(), i));)

		DynamicIn.clear();
		DynamicIn.insert(symbol_union.begin(), i);
	}
	DEBUG_FIXEDNESS(fprintf(stderr, "At start of block %s the following symbols are dynamic,\n", GetName().c_str()););
	for (std::set<SSASymbol *>::const_iterator ci = DynamicIn.begin(); ci != DynamicIn.end(); ++ci) {
		DEBUG_FIXEDNESS(fprintf(stderr, "%s\n", (*ci)->GetName().c_str()););
	}
	DEBUG_FIXEDNESS(fprintf(stderr, "\n"););

	std::set<SSASymbol *> DynamicNow = DynamicIn;
	std::set<SSASymbol *> PrevDynamicOut = DynamicOut;

	// now loop through the block, removing any symbols which are given non-fixed values
	for (SSAStatement *statement : Statements) {
		// ask the statement about any variables which it kills and add them to the DynamicNow
		// set
		{
			std::set<SSASymbol *> killed = statement->GetKilledVariables();
			for (std::set<SSASymbol *>::const_iterator ki = killed.begin(); ki != killed.end(); ++ki) {
				DynamicNow.insert(*ki);
				DEBUG_FIXEDNESS(fprintf(stderr, "Killing %s because of %s (variable killed)\n", (*ki)->GetName().c_str(), (statement)->GetName().c_str()););
			}
		}

		// If this is a variable read...
		if (SSAVariableReadStatement * read = dynamic_cast<SSAVariableReadStatement *> (statement)) {
			// If the variable is const, mark the read as const, otherwise mark it as non const
			read->Const = true;
			for (auto foo : DynamicNow) {
				if (foo == read->Target()) {
					read->Const = false;
					DEBUG_FIXEDNESS(fprintf(stderr, "Marked %s as non const because of %s\n", read->GetName().c_str(), foo->GetName().c_str()););
					break;
				}
			}
			//read->Const = DynamicNow.find(read->Target) == DynamicNow.end();
			DEBUG_FIXEDNESS(fprintf(stderr, "Marked %s, read of %s as %s\n", read->GetName().c_str(), read->Target()->GetName().c_str(), read->Const ? "const" : "non-const"););
		}
		// If this is a variable write...
		else if (SSAVariableWriteStatement * write = dynamic_cast<SSAVariableWriteStatement *> (statement)) {
			// If we write a non const value to a const variable, remove it from the const set
			if (!write->IsFixed()) {
				DynamicNow.insert(write->Target());
				DEBUG_FIXEDNESS(fprintf(stderr, "Killing %s because of %s (variable write)\n", write->Target()->GetName().c_str(), write->GetName().c_str()););
			} else {
				assert(dynamic_cast<SSAMemoryReadStatement *> (statement) == NULL);
			}
		}
	}

	// If we have made no changes to our const set or fixedness, we can return
	DynamicOut = DynamicNow;
	DEBUG_FIXEDNESS(fprintf(stderr, "At end of block %s the following symbols are dynamic,\n", GetName().c_str()););
	for (std::set<SSASymbol *>::const_iterator ci = DynamicOut.begin(); ci != DynamicOut.end(); ++ci) {
		DEBUG_FIXEDNESS(fprintf(stderr, "%s\n", (*ci)->GetName().c_str()););
	}
	DEBUG_FIXEDNESS(fprintf(stderr, "\n"););

	if (DynamicOut != PrevDynamicOut || _constness != prev_constness) {
		return false;
	}

	return true;
}

void SSABlock::ClearFixedness()
{
	DynamicIn.clear();
	DynamicOut.clear();
	_constness = BLOCK_INVALID;
}


void SSABlock::DoInsertTemporaries()
{
	CheckDisposal();
	// loop over the statements in this block. If they are used by a statement from a different
	// block, insert a write of the statement into a temporary variable at the end of this block
	// and a replace the use of the statement with a read from this temporary variable

	std::map<SSAStatement *, SSASymbol *> temporaries;
	std::multimap<SSAStatement *, SSAStatement *> uses; // Map of usees to users
	for (auto s_i = Statements.begin(); s_i != Statements.end(); ++s_i) {
		SSAStatement *stmt = *s_i;
		for (const auto &_use : stmt->GetUses()) {
			SSAStatement *use = dynamic_cast<SSAStatement*> (_use);
			if (use != nullptr) {
				// if this use is a statement in a different block
				if (use->Parent != this) {
					uses.insert(std::make_pair(stmt, use));
				}
			}
		}
	}

	for (std::multimap<SSAStatement *, SSAStatement *>::iterator u_i = uses.begin(); u_i != uses.end(); ++u_i) {
		SSAStatement *stmt = u_i->first;
		SSAStatement *use = u_i->second;

		if (temporaries.find(stmt) == temporaries.end()) {

			temporaries[stmt] = new SSASymbol(GetContext(), "copy_of_" + stmt->GetName(), stmt->GetType(), Symbol_Temporary);
			Parent->AddSymbol(temporaries[stmt]);
			auto vws = new SSAVariableWriteStatement(this, temporaries[stmt], stmt, Statements.back());
			vws->SetDiag(stmt->GetDiag());
		}
		SSAStatement *read = new SSAVariableReadStatement(use->Parent, temporaries[stmt], use);
		read->SetDiag(use->GetDiag());

		use->Replace(stmt, read);
	}
}

void SSABlock::Cleanup()
{
	// Remove any trailing instructions
	bool foundCtrlFlow = false;

	auto i = Statements.begin();
	for (; i != Statements.end(); i++) {
		if((*i)->GetClass() == SSAStatement::Class_Controlflow) {
			foundCtrlFlow = true;
			break;
		}
	}

	// assert(foundCtrlFlow && "All blocks must end with a control flow instruction!");
	if (foundCtrlFlow) {
		// We don't want to delete the control flow instruction, so move one instruction forward
		i++;

		// Now delete all of the trailing instructions from the statement list
		std::list<SSAStatement *> rem_stmts;
		for (auto j = i; j != Statements.end(); ++j) {
			rem_stmts.push_back(*j);
		}

		for (SSAStatement *stmt : rem_stmts) {
			RemoveStatement(*stmt);
			stmt->Dispose();
			delete stmt;
		}
	}
}

void SSABlock::Unlink()
{

}

void SSABlock::Destroy()
{
	while (!Statements.empty()) {
		auto statement = Statements.back();
		RemoveStatement(*statement);
		statement->Dispose();
		delete statement;
	}

	Dispose();
}

bool SSABlock::ContainsStatement(SSAStatement* stmt) const
{
	for (auto i : GetStatements()) {
		if (i == stmt) {
			return true;
		}
	}
	return false;
}

SSAControlFlowStatement *SSABlock::GetControlFlow()
{
	SSAControlFlowStatement *ctrlflow = nullptr;

	if(Statements.empty()) {
		ctrlflow = nullptr;
	} else {
//		return dynamic_cast<SSAControlFlowStatement*>(Statements.back());
		if(Statements.back()->GetClass() == SSAStatement::Class_Controlflow) {
			ctrlflow = (SSAControlFlowStatement*)Statements.back();
		}
	}

	return ctrlflow;
}

const SSAControlFlowStatement *SSABlock::GetControlFlow() const
{
	SSAControlFlowStatement *ctrlflow = nullptr;

	if(Statements.empty()) {
		ctrlflow = nullptr;
	} else {
//		return dynamic_cast<const SSAControlFlowStatement*>(Statements.back());
		if(Statements.back()->GetClass() == SSAStatement::Class_Controlflow) {
			ctrlflow = (SSAControlFlowStatement*)Statements.back();
		}
	}

	return ctrlflow;
}

std::string SSABlock::ToString() const
{
	printers::SSABlockPrinter block_printer(*this);
	return block_printer.ToString();
}
