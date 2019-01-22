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
#include "genC/ssa/statement/SSAIntrinsicStatement.h"
#include "genC/ssa/SSATypeManager.h"
#include "DiagnosticContext.h"
#include "ir/IRAction.h"

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
		class IRExternalAction;
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
			class SSAExternalAction;
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
			FileContents(const std::string &filename);
			FileContents(const std::string &filename, const std::string &filetext);

			std::string Filename;
			pANTLR3_INPUT_STREAM TokenStream;
		};

		class GenCContext
		{
		public:
			typedef std::multiset<ParserOutput, ParserOutputSorter> FileOutputCollection;
			typedef std::map<std::string, FileOutputCollection> OutputCollection;
			typedef ssa::SSAStatement *(*IntrinsicEmitterFn)(const IRIntrinsicAction *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &wordtype);

			GenCContext(const gensim::arch::ArchDescription &arch, const isa::ISADescription &isa, DiagnosticContext &diag_ctx);

			void LoadStandardConstants();

			bool AddFile(std::string filename);
			bool AddFile(const FileContents &filename);
			bool Parse();

			void ParseExecuteAction(std::string name, std::string action_body);
			void ParseHelperAction(std::string prototype, std::string body);

			void PrettyPrint(std::ostringstream &out) const;

			IRCallableAction *GetCallable(const std::string& name) const;
			IRExternalAction *GetExternal(const std::string& name) const;
			IRIntrinsicAction *GetIntrinsic(const std::string& name) const;
			IntrinsicEmitterFn GetIntrinsicEmitter(const std::string& name) const;
			IRHelperAction *GetHelper(const std::string& name) const;

			bool Resolve();
			void LoadIntrinsics();
			void LoadExternalFunctions();
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

			void InsertConstant(std::string name, IRType type, uint32_t value);

			inline std::pair<IRSymbol *, uint32_t> GetConstant(std::string name) const
			{
				return ConstantTable.at(name);
			}

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
			std::map<std::string, std::pair<IRSymbol *, uint32_t> > ConstantTable;
			std::map<std::string, IRType> type_map_;
			std::vector<FileContents> file_list;

			bool Valid;

			std::map<std::string, std::pair<IRIntrinsicAction *, IntrinsicEmitterFn>> IntrinsicTable;
			std::map<std::string, IRHelperAction *> HelperTable;
			std::map<std::string, IRExecuteAction *> ExecuteTable;
			std::map<std::string, IRExternalAction *> ExternalTable;
			std::map<std::string, isa::InstructionDescription*> InstructionTable;

			std::shared_ptr<ssa::SSATypeManager> type_manager_;

			GenCContext(pANTLR3_BASE_TREE file, std::ostringstream &error_stream, gensim::arch::ArchDescription &arch);

			bool Parse_File(pANTLR3_BASE_TREE File);
			bool Parse_Constant(pANTLR3_BASE_TREE Node);
			bool Parse_Typename(pANTLR3_BASE_TREE Node);
			bool Parse_Helper(pANTLR3_BASE_TREE Behaviour);
			bool Parse_Behaviour(pANTLR3_BASE_TREE Action);
			bool Parse_Execute(pANTLR3_BASE_TREE Execute);
			bool Parse_Vector(pANTLR3_BASE_TREE Execute);

			uint64_t Parse_ConstantInt(pANTLR3_BASE_TREE node);

			IRType Parse_Type(pANTLR3_BASE_TREE node);

			IRBody *Parse_Body(pANTLR3_BASE_TREE Body, IRScope &containing_scope, IRScope *override_scope = NULL);
			IRStatement *Parse_Statement(pANTLR3_BASE_TREE node, IRScope &containing_scope);
			IRExpression *Parse_Expression(pANTLR3_BASE_TREE node, IRScope &containing_scope);

			void BuildStructTypes();

			void AddIntrinsic(const std::string& name, const IRType& retty, const IRSignature::param_type_list_t& ptl, IntrinsicEmitterFn emitter, ssa::SSAIntrinsicStatement::IntrinsicType);
			void AddExternalFunction(const std::string& name, const IRType& retty, const IRSignature::param_type_list_t& ptl);
		};
	}
}
