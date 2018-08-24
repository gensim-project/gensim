/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IR.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/Parser.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSAVariableReadStatement::SSAVariableReadStatement(SSABlock *parent, SSASymbol *target, SSAStatement *before)
	: SSAStatement(Class_Unknown, 1, parent, before), Const(true)
{
	SetTarget(target);
}

SSAVariableReadStatement::~SSAVariableReadStatement()
{

}

void SSAVariableReadStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitVariableReadStatement(*this);
}

const SSAType SSAVariableReadStatement::ResolveType(const SSASymbol *target) const
{
	return target->GetType();
}

const SSAVariableWriteStatement *SSAVariableReadStatement::GetDominatingWriteFrom(const std::vector<SSABlock *> &block_list) const
{
	const SSAVariableWriteStatement *stmt = Parent->GetLastWriteTo(Target(), this);
	if (stmt) return stmt;

	std::vector<SSABlock *>::const_reverse_iterator i = block_list.rbegin();
	while (*i != Parent) ++i; // Skip to this block
	++i; // Skip this block

	while (!stmt && (i != block_list.rend())) stmt = (*i++)->GetLastWriteTo(Target());
	return stmt;
}

const SSAVariableWriteStatement *SSAVariableReadStatement::GetFirstDominatingWrite() const
{
	std::list<const SSABlock *> block_list = {Parent};
	std::set<const SSABlock*> blocks_seen;

	while (!block_list.empty()) {
		const SSABlock* block = block_list.front();
		block_list.pop_front();
		if (blocks_seen.count(block)) continue;
		blocks_seen.insert(block);

		const SSAVariableWriteStatement *write = block->GetLastWriteTo(Target());
		if (write) return write;

		for (const auto pred : block->GetPredecessors()) {
			block_list.push_back(pred);
		}
	}

	return NULL;
}

bool SSAVariableReadStatement::HasNoUndefinedPaths()
{
	std::list<SSABlock *> work_list = {Parent};
	std::set<SSABlock *> seen_blocks = {Parent};

	while (!work_list.empty()) {
		SSABlock *curr_block = work_list.front();
		work_list.pop_front();

		SSABlock::StatementListConstReverseIterator iterator;
		if (curr_block == Parent) {
			iterator = Parent->GetStatements().rbegin();
			while (*iterator != this) iterator++;
		} else {
			iterator = curr_block->GetStatements().rbegin();
		}

		bool found_write = false;

		// Look for a write to my target within the current block
		for (; iterator != curr_block->GetStatements().rend(); ++iterator) {
			SSAStatement *stmt = *iterator;

			if (SSAVariableKillStatement * killer = dynamic_cast<SSAVariableKillStatement*> (stmt)) {
				if (killer->Target()->ResolveReferences() == Target()->ResolveReferences()) {
					found_write = true;
					break;
				}
			}
		}

		// If we could not find a write in this block, search through the block's immediate predecessors
		if (!found_write) {
			// If the block has no predecessors, we have found a path with no write
			if (curr_block->GetPredecessors().empty()) return false;
			for (const auto pred : curr_block->GetPredecessors()) {
				if (!seen_blocks.count(pred)) work_list.push_back(pred);
				seen_blocks.insert(pred);
			}
		}
	}

	return true;
}

bool SSAVariableReadStatement::Resolve(DiagnosticContext &ctx)
{
	assert(Target() != nullptr);
	// assert(Target->IsReference() || Parent->Parent.GetDominatingAllocs(Target, this).size() != 0);

	assert(Parent->Parent->Symbols().count(Target()));

	// FIXME: check for reads from undefined variables
	if(Target()->SType == Symbol_Local) {
		// Vector types are a bit weird, don't try and track definedness for
		// these at the moment.
		if(Target()->GetType().VectorWidth == 1) {
			// TODO: this doesn't seem to work correctly
//			if(!HasNoUndefinedPaths()) {
//				ctx.Warning("Read from " + Target()->GetPrettyName() + " may give an undefined result (some paths don't store a value)", GetDiag());
//			}
		}
	}

	while (Target()->IsReference()) {
		Replace(Target(), Target()->GetReferencee());
	}

	return SSAStatement::Resolve(ctx);
}
