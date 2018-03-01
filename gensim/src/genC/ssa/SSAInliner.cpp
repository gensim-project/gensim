/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"
#include "genC/ssa/SSACloner.h"
#include "genC/ssa/SSAInliner.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/statement/SSACallStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/statement/SSAReturnStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/statement/SSARaiseStatement.h"

using namespace gensim::genc::ssa;

#define DPRINTF(...)

void SSAInliner::Inline(SSACallStatement* call_site) const
{
	const DiagNode call_site_diag = call_site->GetDiag();

	SSAFormAction *callee = dynamic_cast<SSAFormAction *>(call_site->Target());
	if (callee == nullptr) {
		throw std::logic_error("Attempting to inline non-SSAFormAction callee");
	}

	SSAFormAction *caller = call_site->Parent->Parent;

	// Inlining procedure:
	// 1. clone all of the blocks of the callee into the caller
	// 2. assign value of parameters and set up references
	// 3. replace uses of call site with reads of return variable
	// 4. break call site block after call site
	// 5. replace call site with jump to entry block of callee
	// 6. replace return statements with jump to remainder of block + write to return variable

	std::map<SSABlock *, SSABlock *> block_map;

	// 1. clone all of the blocks of the callee into the caller
	SSACloneContext ctx (caller);
	SSACloner cloner;
	for(auto callee_block : callee->Blocks) {
		cloner.Clone(callee_block, caller, ctx);
	}

	// 1.5 fix phi nodes
	for(auto stmt : ctx.Statements()) {
		auto phi = dynamic_cast<const SSAPhiStatement*>(stmt.first);
		if(phi != nullptr) {
			auto clonedphi = dynamic_cast<SSAPhiStatement*>(stmt.second);
			for(auto member : phi->Get()) {
				clonedphi->Add(ctx.get((SSAStatement*)member));
			}
		}
	}

	// 2. assign value of parameters and set up references
	InlineParameters(call_site, ctx);
	FlattenReferences(caller);

	SSASymbol *return_symbol = nullptr;
	if(callee->GetPrototype().GetIRSignature().GetType() != IRTypes::Void) {
		return_symbol = new SSASymbol(caller->GetContext(), "_R_" + call_site->GetName(), callee->GetPrototype().GetIRSignature().GetType(), Symbol_Temporary);
		caller->AddSymbol(return_symbol);
	}

	// 4. break call site block after call site
	SSABlock *call_site_block = call_site->Parent;

	SSABlock *post_block = new SSABlock(caller->GetContext(), *caller);

	SSABlock *call_block = call_site->Parent;

	bool moving = false;
	std::vector<SSAStatement *> move_statements;
	for(unsigned i = 0; i < call_block->GetStatements().size(); ++i) {
		SSAStatement *stmt = call_block->GetStatements().at(i);
		if(moving) {
			move_statements.push_back(stmt);
		}
		if(stmt == call_site) {
			moving = true;
		}
	}

	for(auto stmt : move_statements) {
		call_block->RemoveStatement(*stmt);
		stmt->Parent = post_block;
		post_block->AddStatement(*stmt);
	}

	if(return_symbol != nullptr) {
		// 3. replace uses of call site with reads of return variable
		// these are necessarily in the post_block
		// insert a read of this variable at the start of the post block
		SSAVariableReadStatement *rtn_read = new SSAVariableReadStatement(post_block, return_symbol, post_block->GetStatements().front());
		rtn_read->SetDiag(call_site_diag);

		for(SSAStatement *stmt : post_block->GetStatements()) {
			stmt->Replace(call_site, rtn_read);
		}
	}

	// 5. replace call site with jump to entry block of callee
	call_block->RemoveStatement(*call_site);
	call_site->Dispose();
	call_site = nullptr;

	auto call_block_jump_stmt = new SSAJumpStatement(call_block, *ctx.Blocks().at(callee->EntryBlock));
	call_block_jump_stmt->SetDiag(call_site_diag);

	// 6. replace return statements with write to return var & jump to remainder of block
	std::vector<SSAReturnStatement *> callee_return_statements;
	for(auto block : callee->Blocks) {
		for(auto stmt : block->GetStatements()) {
			if(SSAReturnStatement *rtn = dynamic_cast<SSAReturnStatement*>(stmt)) {
				callee_return_statements.push_back(rtn);
			} else if (SSARaiseStatement *raise = dynamic_cast<SSARaiseStatement*>(stmt)) {
				throw std::logic_error("inlining functions with raise statements is not supported");
			}
		}
	}

	for(SSAReturnStatement *rtn : callee_return_statements) {
		if(rtn->Value() != nullptr) {
			auto return_write_stmt = new SSAVariableWriteStatement(ctx.get(rtn->Parent), return_symbol, ctx.get(rtn->Value()), ctx.get(rtn));
			return_write_stmt->SetDiag(rtn->GetDiag());
		}

		auto return_jump_stmt = new SSAJumpStatement(ctx.get(rtn->Parent), *post_block, ctx.get(rtn));
		return_jump_stmt->SetDiag(rtn->GetDiag());

		ctx.get(rtn)->Parent->RemoveStatement(*ctx.get(rtn));
		ctx.get(rtn)->Dispose();
		delete ctx.get(rtn);
	}

	InsertTemporaries(call_block, post_block);

	RemoveEmptyBlocks(caller);
}

void SSAInliner::InlineParameters(SSACallStatement *call_site, SSACloneContext &ctx) const
{
	SSAFormAction *callee = dynamic_cast<SSAFormAction *>(call_site->Target());

	for(unsigned i = 0; i < call_site->ArgCount(); ++i) {
		SSAValue *arg = call_site->Arg(i);
		SSASymbol *param = callee->ParamSymbols.at(i);
		SSASymbol *clonedparam = ctx.get(param);

		if(param->GetType().Reference) {
			if(dynamic_cast<SSASymbol *>(arg) == nullptr) {
				throw std::logic_error("");
			}

			// repoint reference to the parameter passed in
			clonedparam->SetReferencee((SSASymbol*)arg);

		} else {
			// change the cloned parameter symbol type and name
			clonedparam->SType = Symbol_Local;

			if(SSASymbol *sym = dynamic_cast<SSASymbol*>(arg)) {
				arg = new SSAVariableReadStatement(call_site->Parent, sym, call_site);
				((SSAVariableReadStatement *)arg)->SetDiag(call_site->GetDiag());		// TODO: This DIAG should really come from the SSA value
			}

			auto param_write_stmt = new SSAVariableWriteStatement(call_site->Parent, ctx.get(param), (SSAStatement*)arg, call_site);
			param_write_stmt->SetDiag(call_site->GetDiag());	// TODO: This DIAG should really come from the SSA value
		}
	}
}

void SSAInliner::FlattenReferences(SSAFormAction* action) const
{
	for(SSASymbol *sym : action->Symbols()) {
		if(sym->IsReference()) {

			// create a local copy of uses to avoid modifying real uses while
			// iterating through it
			auto uses = ((SSAValue*)sym)->GetUses();
			for(auto usevalue : uses) {
				if(SSAStatement *use = dynamic_cast<SSAStatement*>(usevalue)) {
					SSASymbol *final_referencee = sym;
					while(final_referencee->IsReference()) {
						final_referencee = final_referencee->GetReferencee();
					}
					use->Replace(sym, final_referencee);
				}
			}
		}
	}
}

void SSAInliner::InsertTemporaries(SSABlock* call_block, SSABlock* post_block) const
{
	SSAFormAction *caller = call_block->Parent;

	DPRINTF(stderr, "Beginning an inlining pass\n");
	// 6. fix up statements which are defined in the pre block but used in the
	// post block.
	std::map<SSAStatement *, SSAStatement *> split_use_temporaries;
	for(SSAStatement *stmt : call_block->GetStatements()) {
		// examine uses of the statement
		DPRINTF(stderr, " - examining %s\n", stmt->GetName().c_str());
		auto uses = stmt->GetUses();
		for(SSAValue *use : uses) {
			DPRINTF(stderr, " - - used  by %p", use);
			if(SSAStatement *use_stmt = dynamic_cast<SSAStatement*>(use)) {
				DPRINTF(stderr, " (%s)\n", use_stmt->GetName().c_str());
				if(use_stmt->Parent == call_block) {
					// do nothing
					DPRINTF(stderr, " - - - Same block\n");
				} else if(use_stmt->Parent == post_block) {
					SSAStatement *&read = split_use_temporaries[stmt];
					DPRINTF(stderr, " - - - Different block!\n");
					if(read == nullptr) {
						// create a new symbol
						std::string symname = "tmp_" + stmt->GetName();
						SSASymbol *symbol = new SSASymbol(caller->GetContext(), symname, stmt->GetType(), Symbol_Temporary);
						caller->AddSymbol(symbol);

						auto *write = new SSAVariableWriteStatement(call_block, symbol, stmt, call_block->GetControlFlow());
						DPRINTF(stderr, " - - - - Created write %s\n", write->GetName().c_str());
						read = new SSAVariableReadStatement(post_block, symbol, post_block->GetStatements().front());
						DPRINTF(stderr, " - - - - Created read %s\n", read->GetName().c_str());
					}

					use_stmt->Replace(stmt, read);

				} else {
					// this should never happen
					UNREACHABLE;
				}
			} else {
				DPRINTF(stderr, "\n");
			}
		}
	}
}

void SSAInliner::RemoveEmptyBlocks(SSAFormAction* action) const
{
	const auto *pass = SSAPassDB::Get("DeadPhiElimination");
	while(pass->Run(*action));
	pass = SSAPassDB::Get("UnreachableBlockElimination");
	while(pass->Run(*action));


}
