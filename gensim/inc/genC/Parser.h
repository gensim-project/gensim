/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
/*
 * genC/Parser.h
 *
 * GenSim
 *
 * Harry Wagstaff <hwagstaf@inf.ed.ac.uk>
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
#pragma once

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <queue>
#include <sstream>
#include <string>
#include <memory>

#include <antlr3.h>

#include "genC/ir/IRType.h"
#include "genC/ir/IRSignature.h"
#include "genC/ssa/SSATypeManager.h"
#include "DiagnosticContext.h"
#include "ir/IRAction.h"

#include <flexbison_harness.h>
#include <flexbison_genc_ast.h>
#include <genC.tabs.h>

#include <flexbison_genc.h>

namespace gensim
{

	namespace arch
	{
		class ArchDescription;
	}

	namespace isa
	{
		class ISADescription;
		class InstructionDescription;
	}

	namespace genc
	{

		class IRAction;
		class IRBody;
		class IRStatement;
		class IRExpression;
		class IRHelperAction;
		class IRIntrinsicAction;
		class IRCallableAction;
		class IRExecuteAction;
		class IRCallExpression;
		class IRScope;
		class IRStructType;
		class IRSymbol;
		class IRType;

		namespace ssa
		{
			class SSAContext;
			class SSABuilder;
			class SSAStatement;
		}

		class ParserOutput
		{
		public:

			enum ParserOutputType {
				OUTPUT_ERROR,
				OUTPUT_INFO,
				OUTPUT_WARNING,
				OUTPUT_DEBUG
			};

			ParserOutputType Type;
			uint32_t Line;
			std::string Message;

			ParserOutput(ParserOutputType type, uint32_t line, std::string message) : Type(type), Line(line), Message(message)
			{
			}
		};

		struct ParserOutputSorter {

			bool operator()(const ParserOutput &lhs, const ParserOutput &rhs) const
			{
				return lhs.Line < rhs.Line;
			}
		};

		class FileContents
		{
		public:
			FileContents(const std::string &filename, std::shared_ptr<std::istream> stream);

			std::string Filename;
			std::shared_ptr<std::istream> Stream;
		};

		class GenCContext
		{
		public:
			typedef std::multiset<ParserOutput, ParserOutputSorter> FileOutputCollection;
			typedef std::map<std::string, FileOutputCollection> OutputCollection;

			GenCContext(const gensim::arch::ArchDescription &arch, const isa::ISADescription &isa, DiagnosticContext &diag_ctx);

			void LoadStandardConstants();

			bool AddFile(std::string filename);
			bool AddFile(const FileContents &filename);
			bool Parse();

			void ParseExecuteAction(std::string name, std::string action_body);
			void ParseHelperAction(std::string prototype, std::string body);

			void PrettyPrint(std::ostringstream &out) const;

			IRCallableAction *GetCallable(const std::string& name) const;
			IRIntrinsicAction *GetIntrinsic(const std::string& name) const;
			IRHelperAction *GetHelper(const std::string& name) const;

			bool Resolve();
			void LoadIntrinsics();
			void LoadRegisterNames();
			void LoadFeatureNames();
			ssa::SSAContext *EmitSSA();

			bool RegisterInstruction(isa::InstructionDescription* insn, std::string execute_behaviour);
			isa::InstructionDescription* GetRegisteredInstructionFor(IRAction *exec_action);

			IRScope &GlobalScope;

			bool IsValid() const
			{
				return Valid;
			}

			const gensim::arch::ArchDescription &Arch;
			const isa::ISADescription &ISA;

			void InsertConstant(std::string name, const IRType &type, IRConstant value);
			std::pair<IRSymbol *, IRConstant> GetConstant(std::string name) const;

			std::shared_ptr<ssa::SSATypeManager> GetTypeManager()
			{
				return type_manager_;
			}

			DiagnosticContext &Diag()
			{
				return diag_ctx;
			}
			std::string CurrFilename;
		private:
			DiagnosticContext &diag_ctx;

			OutputCollection Output;
			std::map<std::string, std::pair<IRSymbol *, IRConstant> > ConstantTable;
			std::map<std::string, IRType> type_map_;
			std::vector<FileContents> file_list;

			bool Valid;

			std::map<std::string, IRHelperAction *> HelperTable;
			std::map<std::string, IRExecuteAction *> ExecuteTable;
			std::map<std::string, isa::InstructionDescription*> InstructionTable;

			IntrinsicManager intrinsic_manager_;
			std::map<IntrinsicID, IRIntrinsicAction*> IntrinsicTable;

			std::shared_ptr<ssa::SSATypeManager> type_manager_;

			GenCContext(pANTLR3_BASE_TREE file, std::ostringstream &error_stream, gensim::arch::ArchDescription &arch);

			bool Parse_File(GenC::AstNode &File);
			bool Parse_Constant(GenC::AstNode &Node);
			bool Parse_Typename(GenC::AstNode &Node);
			bool Parse_Helper(GenC::AstNode &Behaviour);
			bool Parse_Behaviour(GenC::AstNode &Action);
			bool Parse_Execute(GenC::AstNode &Execute);
			bool Parse_Vector(GenC::AstNode &Execute);

			uint64_t Parse_ConstantInt(GenC::AstNode &node);

			bool Parse_Type(GenC::AstNode &node, IRType &type);

			IRBody *Parse_Body(GenC::AstNode &Body, IRScope &containing_scope, IRScope *override_scope = NULL);
			IRStatement *Parse_Statement(GenC::AstNode &node, IRScope &containing_scope);
			IRExpression *Parse_Expression(GenC::AstNode &node, IRScope &containing_scope);

			void BuildStructTypes();

		};
	}
}
