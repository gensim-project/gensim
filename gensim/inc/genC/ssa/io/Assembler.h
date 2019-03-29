/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Assembler.h
 * Author: harry
 *
 * Created on 17 July 2017, 10:54
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <map>
#include <set>

#include "DiagnosticContext.h"
#include "define.h"
#include "flexbison_harness.h"
#include "flexbison_gencssa_ast.h"
#include "flexbison_gencssa.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSABlock;
			class SSAContext;
			class SSAFormAction;
			class SSAStatement;
			class SSAPhiStatement;
			class SSASymbol;
			class SSAValue;

			namespace io
			{
				class AssemblyFileContext;
				class AssemblyContext
				{
				public:
					SSAValue *Get(const std::string &valuename)
					{
						if(values_.count(valuename) == 0) return nullptr;
						else return values_.at(valuename);
					}
					void Put(const std::string &valuename, SSAValue *value)
					{
						if(values_.count(valuename) != 0) {
							throw std::logic_error("Duplicate value name in assembly context");
						}

						values_[valuename] = value;
					}

					void ProcessPhiStatements();
					void AddPhiPlaceholder(SSAPhiStatement* stmt, const std::string &placeholder);

				private:
					std::map<const std::string, SSAValue*> values_;
					std::map<SSAPhiStatement*, std::set<std::string>> phi_members_;
				};

				class ContextAssembler
				{
				public:
					void SetTarget(SSAContext *target);
					bool Assemble(AssemblyFileContext &afc, DiagnosticContext &dc);

				private:
					SSAContext *target_;
				};

				class ActionAssembler
				{
				public:
					SSAFormAction *Assemble(const GenCSSA::AstNode &tree, SSAContext &context);
				};

				class BlockAssembler
				{
				public:
					BlockAssembler(AssemblyContext &ctx);
					SSABlock *Assemble(const GenCSSA::AstNode &tree, SSAFormAction &action);

				private:
					AssemblyContext &ctx_;
				};

				class StatementAssembler
				{
				public:
					StatementAssembler(SSAFormAction &action, AssemblyContext &ctx);

					SSAStatement *Assemble(const GenCSSA::AstNode &tree, SSABlock *block);

				private:
					SSASymbol *get_symbol(const GenCSSA::AstNode &sym_id_node);
					SSAStatement *get_statement(const GenCSSA::AstNode &stmt_id_node);
					SSABlock *get_block(const GenCSSA::AstNode &block_id_node);

					SSAStatement *parse_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_bank_reg_read_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_bank_reg_write_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_binary_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_call_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_cast_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_constant_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_devread_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_devwrite_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_if_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_intrinsic_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_jump_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_mem_read_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_mem_write_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_read_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_reg_read_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_reg_write_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_raise_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_return_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_select_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_struct_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_switch_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_unop_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_vextract_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_vinsert_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_write_statement(const GenCSSA::AstNode &tree, SSABlock *block);
					SSAStatement *parse_phi_statement(const GenCSSA::AstNode &tree, SSABlock *block);

					SSAFormAction &action_;
					AssemblyContext &ctx_;
				};
			}
		}
	}
}

#endif /* ASSEMBLER_H */

