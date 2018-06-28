/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <assert.h>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <math.h>
#include <string.h>

#include <antlr3.h>
#include <antlr-ver.h>

#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRRegisterExpression.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSAContext.h"
#include "genC/InstStructBuilder.h"

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"

#include <genC/genCLexer.h>
#include <genC/genCParser.h>

#include "antlr-ver.h"

using namespace gensim::genc;

static bool CanAssign(IRExpression *expr)
{
	// we can assign to:
	//	variables
	//	definitions
	//	vector indexes
	//  bit sequences
	if(dynamic_cast<IRVariableExpression*>(expr) != nullptr) {
		return true;
	} else if(dynamic_cast<IRDefineExpression*>(expr) != nullptr) {
		return true;
	} else {
		IRUnaryExpression *unary_expr = dynamic_cast<IRUnaryExpression*>(expr);

		if(unary_expr != nullptr) {
			if(unary_expr->Type == IRUnaryOperator::Index) {
				return true;
			} else if(unary_expr->Type == IRUnaryOperator::BitSequence) {
				return true;
			}
		}
	}

	return false;
}

IRCallableAction *GenCContext::GetCallable(const std::string& name) const
{
	// Intrinsics take precedence...
	IRCallableAction *action = GetIntrinsic(name);
	if (action != nullptr) return action;

	// Then helper functions (which can hence override externals)
	action = GetHelper(name);
	if (action != nullptr) return action;

	// Finally external functions.
	action = GetExternal(name);
	if (action != nullptr) return action;

	return nullptr;
}

IRHelperAction *GenCContext::GetHelper(const std::string& name) const
{
	if (HelperTable.find(name) != HelperTable.end()) return HelperTable.at(name);
	return nullptr;
}

IRIntrinsicAction *GenCContext::GetIntrinsic(const std::string& name) const
{
	if (IntrinsicTable.find(name) != IntrinsicTable.end()) return IntrinsicTable.at(name).first;
	return nullptr;
}

IRExternalAction *GenCContext::GetExternal(const std::string& name) const
{
	if (ExternalTable.find(name) != ExternalTable.end()) return ExternalTable.at(name);
	return nullptr;
}

GenCContext::IntrinsicEmitterFn GenCContext::GetIntrinsicEmitter(const std::string& name) const
{
	if (IntrinsicTable.find(name) != IntrinsicTable.end()) return IntrinsicTable.at(name).second;
	return nullptr;
}

bool GenCContext::AddFile(std::string filename)
{
	file_list.push_back(FileContents(filename));
	return true;
}

bool GenCContext::AddFile(const FileContents &file)
{
	file_list.push_back(file);
	return true;
}

static pANTLR3_INPUT_STREAM GetFile(const std::string &str)
{
	FILE *f = fopen(str.c_str(), "r");
	if(f == nullptr) {
		return nullptr;
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *data = (char*)malloc(size);
	fread(data, 1, size, f);

	// check data
	for(size_t i = 0; i < size; ++i) {
		char c= data[i];
		if(!isascii(c)) {
			return nullptr;
		}
	}

	return antlr3StringStreamNew((pANTLR3_UINT8)data, ANTLR3_ENC_8BIT, size, (pANTLR3_UINT8)strdup(str.c_str()));
}

bool GenCContext::Parse()
{
	bool success = true;

	for (auto file : file_list) {
		CurrFilename = file.Filename;

		pANTLR3_INPUT_STREAM pts = file.TokenStream;

		if (!pts) {
			Diag().Error(Format("Couldn't open file '%s'", CurrFilename.c_str()), DiagNode(CurrFilename));
			return false;
		}
		pgenCLexer lexer = genCLexerNew(pts);

		pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
		pgenCParser parser = genCParserNew(tstream);

		genCParser_action_file_return arch_return = parser->action_file(parser);

		if (parser->pParser->rec->getNumberOfSyntaxErrors(parser->pParser->rec) > 0 || lexer->pLexer->rec->getNumberOfSyntaxErrors(lexer->pLexer->rec)) {
			Diag().Error("Syntax errors detected in ISA behaviour", DiagNode());
			return false;
		}

		pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(arch_return.tree, ANTLR3_SIZE_HINT);

		success &= Parse_File(nodes->root);

		nodes->free(nodes);
		parser->free(parser);
		tstream->free(tstream);
		lexer->free(lexer);
		pts->free(pts);
	}

	// Register any instructions to their specified execute actions
	for (const auto &insn : ISA.Instructions) {
		success &= RegisterInstruction(insn.second, insn.second->BehaviourName);
	}

	if (success) success &= Resolve();
	Valid = success;

	return Valid;
}

void GenCContext::LoadStandardConstants()
{
	InsertConstant("NAN", IRTypes::Double, __builtin_nanf(""));
	// ConstantTable["NAN"] = std::pair<IRSymbol*, double>(new IRSymbol("NAN", IRTypes::Double, Symbol_Constant, NULL), NAN);
}

void GenCContext::LoadRegisterNames()
{
	// load register file names
	int regbankid = 0;
	for (const auto &bank : Arch.GetRegFile().GetBanks()) {
		InsertConstant(bank->ID, IRTypes::UInt32, regbankid++);
	}

	// now load individual registers
	regbankid = 0;
	for (const auto &slot : Arch.GetRegFile().GetSlots()) {
		InsertConstant(slot->GetID(), IRTypes::UInt32, regbankid++);
	}

	// also load memory interface names
	regbankid = 0;
	for(const auto &interface : Arch.GetMemoryInterfaces().GetInterfaces()) {
		InsertConstant(interface.first, IRTypes::UInt32, interface.second.GetID());
	}
}

void GenCContext::LoadFeatureNames()
{
	// load register file names
	int regbankid = 0;
	for (const auto &feature : Arch.GetFeatures()) {
		InsertConstant(feature.GetName(), IRTypes::UInt32, feature.GetId());
	}
}

bool GenCContext::Resolve()
{
	bool success = true;
	for (std::map<std::string, IRHelperAction *>::iterator i = HelperTable.begin(); i != HelperTable.end(); ++i) {
		success &= i->second->body->Resolve(*this);
	}

	for (std::map<std::string, IRExecuteAction *>::iterator i = ExecuteTable.begin(); i != ExecuteTable.end(); ++i) {
		bool resolved = i->second->body->Resolve(*this);

		success &= resolved;
	}
	return success;
}

ssa::SSAContext *GenCContext::EmitSSA()
{
	if (!Valid) return NULL;

	auto context = new ssa::SSAContext(ISA, Arch);

	for(auto i : StructTypeTable) {
		context->GetTypeManager().InsertStructType(i.first, i.second);
	}

	for (std::map<std::string, IRHelperAction *>::const_iterator ci = HelperTable.begin(); ci != HelperTable.end(); ++ci) {
		auto ssa_form = ci->second->GetSSAForm(*context);
		context->AddAction(ssa_form);
		GASSERT(ssa_form->HasAttribute(ActionAttribute::Helper));
	}

	for (std::map<std::string, IRExecuteAction *>::const_iterator ci = ExecuteTable.begin(); ci != ExecuteTable.end(); ++ci) {
		context->AddAction(ci->second->GetSSAForm(*context));
	}

	return context;
}


void GenCContext::Build_Inst_Struct()
{
	gensim::genc::InstStructBuilder isb;
	auto structType = isb.BuildType(&ISA);

	StructTypeTable["Instruction"] = structType;
}

FileContents::FileContents(const std::string& filename) : Filename(filename), TokenStream(GetFile(filename))
{

}

FileContents::FileContents(const std::string& filename, const std::string& filetext) : Filename(filename)
{
	TokenStream = antlr3StringStreamNew((pANTLR3_UINT8)filetext.c_str(), ANTLR3_ENC_8BIT, filetext.length(), (pANTLR3_UINT8)filename.c_str());
}


GenCContext::GenCContext(const gensim::arch::ArchDescription &arch, const isa::ISADescription &isa, DiagnosticContext &diag_ctx) : Valid(true), Arch(arch), ISA(isa), GlobalScope(IRScope::CreateGlobalScope(*this)), diag_ctx(diag_ctx)
{
	// build the instruction structure
	Build_Inst_Struct();

	// Load register and register file names
	LoadRegisterNames();
	LoadFeatureNames();
	LoadStandardConstants();
	LoadIntrinsics();
	LoadExternalFunctions();
}

bool GenCContext::RegisterInstruction(isa::InstructionDescription* insn, std::string execute)
{
	assert(insn != NULL);
	assert(&insn->ISA == &ISA);

	if (ExecuteTable.find(execute) == ExecuteTable.end()) {
		Diag().Error("Could not find Execute action " + execute, DiagNode(CurrFilename));
		return false;
	}
	IRExecuteAction* action = ExecuteTable.at(execute);
	if (InstructionTable.find(action->GetSignature().GetName()) != InstructionTable.end()) {
		Diag().Error("Attempted to attach an execute action " + execute + " to multiple instructions", DiagNode(CurrFilename));
		return false;
	}
	assert(InstructionTable.find(action->GetSignature().GetName()) == InstructionTable.end());
	InstructionTable[action->GetSignature().GetName()] = insn;
	return true;
}

gensim::isa::InstructionDescription *GenCContext::GetRegisteredInstructionFor(IRAction *action)
{
	IRExecuteAction *exec_action = dynamic_cast<IRExecuteAction*> (action);
	if (!exec_action || InstructionTable.find(exec_action->GetSignature().GetName()) == InstructionTable.end()) {
		return NULL;
	}
	return InstructionTable.at(exec_action->GetSignature().GetName());
}

void GenCContext::InsertConstant(std::string name, IRType type, uint32_t value)
{
	IRSymbol *sym = GlobalScope.InsertSymbol(name, type, Symbol_Constant);
	ConstantTable[name] = std::make_pair(sym, value);
}

bool GenCContext::Parse_File(pANTLR3_BASE_TREE File)
{
	assert(File->getType(File) == FILETOK);

	bool success = true;
	for (uint32_t i = 0; i < File->getChildCount(File); ++i) {
		pANTLR3_BASE_TREE node = (pANTLR3_BASE_TREE) File->getChild(File, i);
		switch (node->getType(node)) {
			case CONSTANT:
				success &= Parse_Constant(node);
				break;
			case HELPER:
				success &= Parse_Helper(node);
				break;
			case EXECUTE:
				success &= Parse_Execute(node);
				break;
			case BEHAVIOUR:
				success &= Parse_Behaviour(node);
				break;
			default:
				Diag().Error("Parse error: Expected helper, execute or behaviour node", DiagNode(CurrFilename));
				success = false;
				break;
		}
	}

	return success;
}

bool GenCContext::Parse_Execute(pANTLR3_BASE_TREE Execute)
{
	assert(Execute->getType(Execute) == EXECUTE);
	assert(Execute->getChildCount(Execute) == 2);

	pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE) Execute->getChild(Execute, 0);
	pANTLR3_BASE_TREE bodyNode = (pANTLR3_BASE_TREE) Execute->getChild(Execute, 1);

	std::string nameStr = (char *) nameNode->getText(nameNode)->chars;

	if (ExecuteTable.count(nameStr)) {
		diag_ctx.Error("Execute action " + nameStr + " redefined", DiagNode("", nameNode));
		return false;
	}

	if(GetIntrinsic(nameStr) != nullptr) {
		diag_ctx.Error("The name " + nameStr + " is reserved by an intrinsic function", DiagNode("", nameNode));
		return false;
	}

	if(GetExternal(nameStr) != nullptr) {
		diag_ctx.Error("The name " + nameStr + " is reserved by an external function", DiagNode("", nameNode));
		return false;
	}

	IRExecuteAction *execute = new IRExecuteAction(nameStr, *this, StructTypeTable["Instruction"]);
	execute->SetDiag(DiagNode("", Execute));

	IRBody *body = Parse_Body(bodyNode, *execute->GetFunctionScope());
	if (body == NULL) return false;
	execute->body = body;

	IRStatement *maybe_terminator = body->Statements.empty() ? NULL : body->Statements.back();
	IRFlowStatement *terminator = dynamic_cast<IRFlowStatement*> (maybe_terminator);
	if (!terminator) {
		//		Diag().Warning("Execute action " + nameStr + " does not end with a return statement", CurrFilename, nameNode);

		terminator = new IRFlowStatement(*execute->GetFunctionScope());
		terminator->Type = IRFlowStatement::FLOW_RETURN_VOID;
		body->Statements.push_back(terminator);
	}

	if (execute->body == NULL) {
		Diag().Error(Format("Parse error in %s", nameStr.c_str()), DiagNode(CurrFilename, Execute));
		return false;
	}

	ExecuteTable[execute->GetSignature().GetName()] = execute;
	return true;
}

bool GenCContext::Parse_Behaviour(pANTLR3_BASE_TREE Action)
{
	// Do nothing since behaviour actions are handled elsewhere
	return true;
}

bool GenCContext::Parse_Constant(pANTLR3_BASE_TREE Node)
{
	assert(Node->getType(Node) == CONSTANT);
	assert(Node->getChildCount(Node) == 2);

	pANTLR3_BASE_TREE declarationNode = (pANTLR3_BASE_TREE) Node->getChild(Node, 0);
	pANTLR3_BASE_TREE constantNode = (pANTLR3_BASE_TREE) Node->getChild(Node, 1);

	pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE) declarationNode->getChild(declarationNode, 0);
	pANTLR3_BASE_TREE idNode = (pANTLR3_BASE_TREE) declarationNode->getChild(declarationNode, 1);

	IRType type = Parse_Type(typeNode);
	type.Const = true;
	std::string id = (char *) (idNode->getText(idNode)->chars);
	uint32_t constant = Parse_ConstantInt(constantNode);

	InsertConstant(id, type, constant);

	return true;
}

bool GenCContext::Parse_Helper(pANTLR3_BASE_TREE node)
{
	assert(node->getType(node) == HELPER);

	pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
	pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);
	pANTLR3_BASE_TREE scopeNode = (pANTLR3_BASE_TREE) node->getChild(node, 2);
	pANTLR3_BASE_TREE paramsNode = (pANTLR3_BASE_TREE) node->getChild(node, 3);

	if (scopeNode->getType(scopeNode) == PARAMS) {
		paramsNode = scopeNode;
		scopeNode = NULL;
	}

	std::string helpername = std::string((char*) nameNode->getText(nameNode)->chars);
	if (HelperTable.count(helpername)) {
		diag_ctx.Error("Helper function " + helpername + " redefined", DiagNode("", nameNode));
		return false;
	}

	pANTLR3_BASE_TREE bodyNode = (pANTLR3_BASE_TREE) node->getChild(node, node->getChildCount(node) - 1);

	std::vector<IRParam> param_list;

	for (uint32_t i = 0; i < paramsNode->getChildCount(paramsNode); ++i) {
		pANTLR3_BASE_TREE paramNode = (pANTLR3_BASE_TREE) paramsNode->getChild(paramsNode, i);

		pANTLR3_BASE_TREE paramTypeNode = (pANTLR3_BASE_TREE) paramNode->getChild(paramNode, 0);
		pANTLR3_BASE_TREE paramNameNode = (pANTLR3_BASE_TREE) paramNode->getChild(paramNode, 1);

		std::string paramName = (char *) paramNameNode->getText(paramNameNode)->chars;

		IRType paramType = Parse_Type(paramTypeNode);

		param_list.push_back(IRParam(paramName, paramType));
	}

	bool inlined = false;
	bool noinlined = false;
	bool exported = false;
	IRConstness::IRConstness helper_constness = IRConstness::never_const;
	for (uint32_t i = 3; i < node->getChildCount(node) - 1; ++i) {

		pANTLR3_BASE_TREE attrNode = (pANTLR3_BASE_TREE) node->getChild(node, i);
		switch (attrNode->getType(attrNode)) {
			case GENC_ATTR_INLINE: {
				inlined = true;
				break;
			}
			case GENC_ATTR_NOINLINE: {
				noinlined = true;
				break;
			}
			case GENC_ATTR_EXPORT: {
				exported = true;
				break;
			}
		}
	}

	HelperScope scope = InternalScope;
	if (scopeNode != NULL) {
		switch (scopeNode->getType(scopeNode)) {
			case GENC_SCOPE_PRIVATE:
				scope = PrivateScope;
				break;
			case GENC_SCOPE_INTERNAL:
				scope = InternalScope;
				break;
			case GENC_SCOPE_PUBLIC:
				scope = PublicScope;
				break;
		}
	}

	IRType return_type = Parse_Type(typeNode);
	if(return_type.Reference) {
		Diag().Error("Cannot return a reference from a helper function");
		return false;
	}

	std::string name = (char *) nameNode->getText(nameNode)->chars;
	IRSignature sig(name, return_type, param_list);
	if (noinlined) {
		sig.AddAttribute(ActionAttribute::NoInline);
	}
	if(exported) {
		sig.AddAttribute(ActionAttribute::Export);
	}
	sig.AddAttribute(ActionAttribute::Helper);

	IRHelperAction *helper = new IRHelperAction(sig, scope, *this);

	helper->body = Parse_Body(bodyNode, *helper->GetFunctionScope());
	if (helper->body == NULL) return false;

	HelperTable[helper->GetSignature().GetName()] = helper;

	return true;
}

IRType GenCContext::Parse_Type(pANTLR3_BASE_TREE node)
{
	assert(node->getType(node) == TYPE);
	assert(node->getChildCount(node) >= 1);

	pANTLR3_BASE_TREE baseNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);

	std::string baseType = (char *) baseNode->getText(baseNode)->chars;

	IRType type(IRTypes::Void);

	if (baseType == "uint8") {
		type = IRTypes::UInt8;
	} else if (baseType == "sint8") {
		type = IRTypes::Int8;
	} else if (baseType == "uint16") {
		type = IRTypes::UInt16;
	} else if (baseType == "sint16") {
		type = IRTypes::Int16;
	} else if (baseType == "uint32") {
		type = IRTypes::UInt32;
	} else if (baseType == "sint32") {
		type = IRTypes::Int32;
	} else if (baseType == "uint64") {
		type = IRTypes::UInt64;
	} else if (baseType == "sint64") {
		type = IRTypes::Int64;
	} else if (baseType == "uint128") {
		type = IRTypes::UInt128;
	} else if (baseType == "sint128") {
		type = IRTypes::Int128;
	} else if (baseType == "float") {
		type = IRTypes::Float;
	} else if (baseType == "double") {
		type = IRTypes::Double;
	} else if (baseType == "longdouble") {
		type = IRTypes::LongDouble;
	} else if (baseType == "void") {
		type = IRTypes::Void;
	} else if (baseType == "Instruction") {
		type = StructTypeTable["Instruction"];
	} else {
		Diag().Error("Unrecognized base type " + baseType, DiagNode(CurrFilename, node));
		type = IRTypes::Void;
	}

	for (uint32_t i = 1; i < node->getChildCount(node); ++i) {
		pANTLR3_BASE_TREE annotationNode = (pANTLR3_BASE_TREE) node->getChild(node, i);

		std::string annotation = (char *) annotationNode->getText(annotationNode)->chars;
		if (annotation == "&") {
			type.Reference = true;
		} else if (annotationNode->getType(annotationNode) == VECTOR) {
			pANTLR3_BASE_TREE widthNode = (pANTLR3_BASE_TREE) annotationNode->getChild(annotationNode, 0);
			type.VectorWidth = strtol((char*) widthNode->getText(widthNode)->chars, NULL, 10);
		} else {
			Diag().Error("Unrecognized type annotation " + annotation, DiagNode(CurrFilename, node));
		}
	}

	return type;
}

IRBody *GenCContext::Parse_Body(pANTLR3_BASE_TREE node, IRScope &containing_scope, IRScope *override_scope)
{
	assert(node->getType(node) == BODY);
	IRBody *body;
	if (override_scope != NULL)
		body = IRBody::CreateBodyWithScope(*override_scope);
	else
		body = new IRBody(containing_scope);

	body->SetDiag(DiagNode(CurrFilename, node));

	for (uint32_t i = 0; i < node->getChildCount(node); ++i) {
		pANTLR3_BASE_TREE statementNode = (pANTLR3_BASE_TREE) node->getChild(node, i);
		IRStatement *statement = Parse_Statement(statementNode, (body->Scope));
		if (statement == NULL) return NULL;

		body->Statements.push_back(statement);

		if (auto ret = dynamic_cast<IRFlowStatement*> (statement)) {
			if (ret->Type == IRFlowStatement::FLOW_RETURN_VALUE || ret->Type == IRFlowStatement::FLOW_RETURN_VOID) {
				if (i < (node->getChildCount(node) - 1)) {
					Diag().Error("Return statement should be at end of block", DiagNode(CurrFilename, node));
					return NULL;
				}
			}
		}
	}

	return body;
}

IRStatement *GenCContext::Parse_Statement(pANTLR3_BASE_TREE node, IRScope &containing_scope)
{
	switch (node->getType(node)) {
		case BODY: {
			return Parse_Body(node, containing_scope, NULL);
		}
		case CASE: {
			if (containing_scope.Type != IRScope::SCOPE_SWITCH) {
				Diag().Error("Case not in switch statement", DiagNode(CurrFilename, node));
				break;
			}
			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node));
			c->Type = IRFlowStatement::FLOW_CASE;
			c->Expr = Parse_Expression((pANTLR3_BASE_TREE) node->getChild(node, 0), containing_scope);
			if(c->Expr == nullptr) {
				return nullptr;
			}
			if(dynamic_cast<IRConstExpression*>(c->Expr) == nullptr) {
				Diag().Error("Case statement expression must be a constant\n", DiagNode(CurrFilename, node));
				return nullptr;
			}

			IRScope *CaseScope = new IRScope(containing_scope, IRScope::SCOPE_CASE);
			c->Body = Parse_Body((pANTLR3_BASE_TREE) node->getChild(node, 1), containing_scope, CaseScope);
			if (c->Body == NULL) return NULL;
			return c;
		}
		case DEFAULT: {
			if (containing_scope.Type != IRScope::SCOPE_SWITCH) {
				Diag().Error("Default not in switch statement", DiagNode(CurrFilename, node));
				break;
			}

			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node));
			c->Type = IRFlowStatement::FLOW_DEFAULT;
			IRScope *CaseScope = new IRScope(containing_scope, IRScope::SCOPE_CASE);
			c->Body = Parse_Body((pANTLR3_BASE_TREE) node->getChild(node, 0), containing_scope, CaseScope);
			if (c->Body == NULL) return NULL;
			return c;
		}
		case BREAK: {
			if (!containing_scope.ScopeBreakable()) {
				Diag().Error("Break not in breakable context", DiagNode(CurrFilename, node));
				break;
			}
			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node));
			c->Type = IRFlowStatement::FLOW_BREAK;
			return c;
		}
		case CONTINUE: {
			if (!containing_scope.ScopeContinuable()) {
				Diag().Error("Continue not in continuable context", DiagNode(CurrFilename, node));
				break;
			}
			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node));
			c->Type = IRFlowStatement::FLOW_CONTINUE;
			return c;
		}
		case RAISE: {
			IRFlowStatement *ret = new IRFlowStatement(containing_scope);
			ret->SetDiag(DiagNode(CurrFilename, node));
			ret->Type = IRFlowStatement::FLOW_RAISE;

			return ret;
		}
		case RETURN: {
			IRFlowStatement *ret = new IRFlowStatement(containing_scope);
			ret->SetDiag(DiagNode(CurrFilename, node));

			if (node->getChildCount(node) != 0) {
				if(!containing_scope.GetContainingAction().GetSignature().HasReturnValue()) {
					Diag().Error("Tried to return a value from a function with a void return type", ret->Diag());
					return nullptr;
				}

				ret->Type = IRFlowStatement::FLOW_RETURN_VALUE;
				ret->Expr = Parse_Expression((pANTLR3_BASE_TREE) node->getChild(node, 0), containing_scope);
				if(ret->Expr == nullptr) {
					return nullptr;
				}
			} else {
				if(containing_scope.GetContainingAction().GetSignature().HasReturnValue()) {
					Diag().Error("Tried to return void from a function with a non-void return type", ret->Diag());
					return nullptr;
				}
				ret->Type = IRFlowStatement::FLOW_RETURN_VOID;
			}

			return ret;
		}
		case WHILE: {
			pANTLR3_BASE_TREE exprNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE bodyNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_LOOP);
			auto expr = Parse_Expression(exprNode, containing_scope);
			if(expr == nullptr) {
				return nullptr;
			}
			auto stmt = Parse_Statement(bodyNode, containing_scope);
			if(stmt == nullptr) {
				return nullptr;
			}
			IRIterationStatement *iter = IRIterationStatement::CreateWhile(*scope, *expr, *stmt);
			iter->SetDiag(DiagNode(CurrFilename, node));
			return iter;
		}
		case DO: {
			pANTLR3_BASE_TREE exprNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);
			pANTLR3_BASE_TREE bodyNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_LOOP);

			IRExpression *expr = Parse_Expression(exprNode, *scope);
			if(expr == nullptr) {
				return nullptr;
			}
			IRStatement *body = Parse_Statement(bodyNode, *scope);
			if(body == nullptr) {
				return nullptr;
			}

			IRIterationStatement *iter = IRIterationStatement::CreateDoWhile(*scope, *expr, *body);
			iter->SetDiag(DiagNode(CurrFilename, node));
			return iter;
		}
		case FOR: {
			pANTLR3_BASE_TREE initNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE checkNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);
			pANTLR3_BASE_TREE postNode = (pANTLR3_BASE_TREE) node->getChild(node, 2);
			pANTLR3_BASE_TREE bodyNode = (pANTLR3_BASE_TREE) node->getChild(node, 3);

			IRExpression *start = Parse_Expression(initNode, containing_scope);
			IRExpression *check = Parse_Expression(checkNode, containing_scope);
			IRExpression *post = Parse_Expression(postNode, containing_scope);

			if(start == nullptr || check == nullptr || post == nullptr) {
				return nullptr;
			}

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_LOOP);

			IRIterationStatement *iter = IRIterationStatement::CreateFor(*scope, *start, *check, *post, *Parse_Statement(bodyNode, containing_scope));
			iter->SetDiag(DiagNode(CurrFilename, node));
			return iter;
		}
		case IF: {
			pANTLR3_BASE_TREE exprNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE thenNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			IRSelectionStatement *stmt;

			auto expr = Parse_Expression(exprNode, containing_scope);
			auto then_stmt = Parse_Statement(thenNode, containing_scope);

			if(dynamic_cast<IRDefineExpression*>(expr)) {
				Diag().Error("Cannot define variable in if statement expression", DiagNode(CurrFilename, node));
				return nullptr;
			}

			if (node->getChildCount(node) == 3) {
				pANTLR3_BASE_TREE elseNode = (pANTLR3_BASE_TREE) node->getChild(node, 2);
				auto else_stmt = Parse_Statement(elseNode, containing_scope);
				if(expr == nullptr || then_stmt == nullptr || else_stmt == nullptr) {
					return nullptr;
				}

				stmt = new IRSelectionStatement(containing_scope, IRSelectionStatement::SELECT_IF, *expr, *then_stmt, else_stmt);
			} else {
				if(expr == nullptr || then_stmt == nullptr) {
					return nullptr;
				}
				stmt = new IRSelectionStatement(containing_scope, IRSelectionStatement::SELECT_IF, *expr, *then_stmt, NULL);
			}

			stmt->SetDiag(DiagNode(CurrFilename, exprNode));
			return stmt;
		}
		case SWITCH: {
			pANTLR3_BASE_TREE exprNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE bodyNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_SWITCH);

			auto expr = Parse_Expression(exprNode, containing_scope);
			auto body = Parse_Body(bodyNode, containing_scope, scope);

			if(expr == nullptr || body == nullptr) {
				return nullptr;
			}

			// check body to ensure that it only contains case and default statements
			for(auto bodystmt : body->Statements) {
				IRFlowStatement *flow_stmt = dynamic_cast<IRFlowStatement*>(bodystmt);
				if(!flow_stmt || (flow_stmt->Type != IRFlowStatement::FLOW_CASE && flow_stmt->Type != IRFlowStatement::FLOW_DEFAULT)) {
					Diag().Error("A switch statement body can only contain case and default statements", bodystmt->Diag());
					return nullptr;
				}
			}

			IRSelectionStatement *sel = new IRSelectionStatement(containing_scope, IRSelectionStatement::SELECT_SWITCH, *expr, *body, NULL);
			sel->SetDiag(DiagNode(CurrFilename, exprNode));

			return sel;
		}
		case EXPR_STMT: {
			pANTLR3_BASE_TREE exprNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);

			auto expression = Parse_Expression(exprNode, containing_scope);
			if(expression == nullptr) {
				return nullptr;
			}

			IRExpressionStatement *expr = new IRExpressionStatement(containing_scope, *expression);
			expr->SetDiag(DiagNode(CurrFilename, node));
			return expr;
		}
		default: {
			throw std::logic_error("Unknown node type");
		}
	}
	return NULL;
}

uint64_t GenCContext::Parse_ConstantInt(pANTLR3_BASE_TREE node)
{
	switch (node->getType(node)) {
		case INT_CONST:
			return atoi((char *) (node->getText(node)->chars));
		case HEX_VAL:
			int64_t value;
			uint64_t uvalue;
			sscanf((char *) node->getText(node)->chars, "0x%lx", &uvalue);

			value = (int64_t) uvalue;

			return value;
	}
	assert(false && "Unrecognized constant");
	UNEXPECTED;
}

static bool can_be_int32(uint64_t v)
{
	return v <= std::numeric_limits<int32_t>::max();
}

static bool can_be_int64(uint64_t v)
{
	return v <= std::numeric_limits<int64_t>::max();
}

IRExpression *GenCContext::Parse_Expression(pANTLR3_BASE_TREE node, IRScope &containing_scope)
{
	switch (node->getType(node)) {
		// BODY node type represents empty expression
		case BODY: {
			assert(node->getChildCount(node) == 0);
			EmptyExpression *expr = new EmptyExpression(containing_scope);
			expr->SetDiag(DiagNode(CurrFilename, node));
			return expr;
		}
		case INT_CONST:
		case HEX_VAL: {
			assert(node->getChildCount(node) == 0);

			// final type of constant depends on if it has any type length specifiers
			bool unsigned_specified = false;
			bool long_specified = false;
			bool is_signed = true;
			bool is_long = false;

			std::string text = (char *) node->getText(node)->chars;
			for (auto i : text) {
				if (isdigit(i)) continue;
				else if (tolower(i) == 'u') {
					is_signed = false;
					unsigned_specified = true;
				} else if (tolower(i) == 'l') {
					is_long = true;
					long_specified = true;
				} else {
					UNREACHABLE;
				}
			}

			uint64_t value = strtoull(text.c_str(), nullptr, 0);

			// figure out the final signedness and longness
			if(!long_specified && !unsigned_specified) {
				// does it fit in a int32
				if(can_be_int32(value)) {
					is_signed = true;
					is_long = false;
				} else if(can_be_int64(value)) {
					is_signed = true;
					is_long = true;
				} else {
					is_signed = false;
					is_long = true;
				}
			}

			uint64_t final_value;

			// Figure out smallest value to store valuedd
			IRType type;

			if (is_signed) {
				if (is_long) {
					type = IRTypes::Int64;
					final_value = value;
				} else {
					type = IRTypes::Int32;
					final_value = value & 0xffffffff;
				}
			} else {
				if (is_long) {
					type = IRTypes::UInt64;
					final_value = value;
				} else {
					type = IRTypes::UInt32;
					final_value = value & 0xffffffff;
				}
			}

			type.Const = true;

			IRConstant iv = IRConstant::Integer(final_value);
			if (IRType::Cast(iv, type, type) != iv) {
				Diag().Warning("Constant value truncated", DiagNode(CurrFilename, node));
			}
			auto expr = new IRConstExpression(containing_scope, type, iv);
			expr->SetDiag(DiagNode(CurrFilename, node));
			return expr;
		}

		case FLOAT_CONST: {
			assert(node->getChildCount(node) == 0);
			double value = atof((char *) node->getText(node)->chars);
			IRType Type = IRTypes::Double;
			Type.Const = true;
			auto expr = new IRConstExpression(containing_scope, Type, IRConstant::Double(value));
			expr->SetDiag(DiagNode(CurrFilename, node));
			return expr;
		}
		case GENC_ID: {
			assert(node->getChildCount(node) == 0);
			std::string id = (char *) node->getText(node)->chars;

			IRVariableExpression *expr = new IRVariableExpression(id, containing_scope);
			expr->SetDiag(DiagNode(CurrFilename, node));

			return expr;
		}
		case DECL: {
			assert(node->getChildCount(node) == 2);
			pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			IRType type = Parse_Type(typeNode);
			std::string name = (char *) nameNode->getText(nameNode)->chars;

			IRDefineExpression *expr = new IRDefineExpression(containing_scope, type, name);
			expr->SetDiag(DiagNode(CurrFilename, node));

			return expr;
		}

		case CALL: {
			pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);

			IRCallExpression *call;

			// TODO: these should be handled as intrinsics
			std::string targetName = (char *) nameNode->getText(nameNode)->chars;
			if (targetName == "read_register")
				call = new IRRegisterSlotReadExpression(containing_scope);
			else if (targetName == "write_register")
				call = new IRRegisterSlotWriteExpression(containing_scope);
			else if (targetName == "read_register_bank")
				call = new IRRegisterBankReadExpression(containing_scope);
			else if (targetName == "write_register_bank")
				call = new IRRegisterBankWriteExpression(containing_scope);
			else
				call = new IRCallExpression(containing_scope);

			call->SetDiag(DiagNode(CurrFilename, node));
			call->TargetName = targetName;

			bool success = true;
			for (uint32_t i = 1; i < node->getChildCount(node); ++i) {
				IRExpression *argexpr = Parse_Expression((pANTLR3_BASE_TREE) node->getChild(node, i), containing_scope);
				if(argexpr == nullptr) {
					success = false;
				}
				if(dynamic_cast<IRDefineExpression*>(argexpr)) {
					Diag().Error("Cannot define a variable here.", argexpr->Diag()); //TODO: this should be a syntax error
					success = false;
				}
				call->Args.push_back(argexpr);
			}

			if(success) {
				return call;
			} else {
				return nullptr;
			}
		}

		case CAST: {
			assert(node->getChildCount(node) == 2);

			pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE innerNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			assert(typeNode->getType(typeNode) == TYPE);

			IRExpression *innerExpr = Parse_Expression(innerNode, containing_scope);
			if(innerExpr == nullptr) {
				return nullptr;
			}

			//TODO: this should be a syntax error
			if(dynamic_cast<IRDefineExpression*>(innerExpr)) {
				Diag().Error("Cannot cast a definition", DiagNode(CurrFilename, node));
				return nullptr;
			}

			IRCastExpression *gce = new IRCastExpression(containing_scope, Parse_Type(typeNode));
			gce->SetDiag(DiagNode(CurrFilename, node));
			gce->Expr = innerExpr;

			return gce;
		}

		case PREFIX: {
			assert(node->getChildCount(node) == 2);

			pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE innerNode = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			IRUnaryExpression *unary = new IRUnaryExpression(containing_scope);

			std::string typeString = (char *) typeNode->getText(typeNode)->chars;
			unary->BaseExpression = Parse_Expression(innerNode, containing_scope);
			if(unary->BaseExpression == nullptr) {
				return nullptr;
			}

			if (typeString == "++")
				unary->Type = IRUnaryOperator::Preincrement;
			else if (typeString == "--")
				unary->Type = IRUnaryOperator::Predecrement;
			else if (typeString == "!")
				unary->Type = IRUnaryOperator::Negate;
			else if (typeString == "-")
				unary->Type = IRUnaryOperator::Negative;
			else if (typeString == "~")
				unary->Type = IRUnaryOperator::Complement;
			else if (typeString == "+") {
				unary->Type = IRUnaryOperator::Positive;
			} else {
				Diag().Error("Unrecognized prefix operator " + typeString, DiagNode(CurrFilename, node));
				return nullptr;
			}

			if(unary->Type == IRUnaryOperator::Preincrement || unary->Type == IRUnaryOperator::Predecrement) {
				if(dynamic_cast<IRVariableExpression*>(unary->BaseExpression) == nullptr) {
					Diag().Error("Operator can only be applied to a variable");
					return nullptr;
				}
			}

			unary->SetDiag(DiagNode(CurrFilename, node));
			return unary;
		}
		case POSTFIX: {
			pANTLR3_BASE_TREE innerNode = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			IRExpression *innerExpression = Parse_Expression(innerNode, containing_scope);
			if(innerExpression == nullptr) {
				return nullptr;
			}
			for (uint32_t i = 1; i < node->getChildCount(node); ++i) {
				pANTLR3_BASE_TREE postfixNode = (pANTLR3_BASE_TREE) node->getChild(node, i);
				std::string op = (char *) postfixNode->getText(postfixNode)->chars;

				IRUnaryExpression *unaryExpr = new IRUnaryExpression(containing_scope);
				unaryExpr->BaseExpression = innerExpression;


				switch (postfixNode->getType(postfixNode)) {
					case IDX: {
						pANTLR3_BASE_TREE index_expression = (pANTLR3_BASE_TREE) postfixNode->getChild(postfixNode, 0);

						if (index_expression->getType(index_expression) == IDX_ELEM) {
							pANTLR3_BASE_TREE argNode = (pANTLR3_BASE_TREE) index_expression->getChild(index_expression, 0);
							unaryExpr->Type = IRUnaryOperator::Index;
							unaryExpr->Arg = Parse_Expression(argNode, containing_scope);
						} else if (index_expression->getType(index_expression) == IDX_BITS) {
							pANTLR3_BASE_TREE from_node = (pANTLR3_BASE_TREE) index_expression->getChild(index_expression, 0);
							pANTLR3_BASE_TREE to_node = (pANTLR3_BASE_TREE) index_expression->getChild(index_expression, 1);

							unaryExpr->Type = IRUnaryOperator::BitSequence;
							unaryExpr->Arg = new IRConstExpression(containing_scope, IRTypes::Int32, IRConstant::Integer(Parse_ConstantInt(from_node)));
							unaryExpr->Arg2 = new IRConstExpression(containing_scope, IRTypes::Int32, IRConstant::Integer(Parse_ConstantInt(to_node)));
						} else {
							assert(false);
						}

						break;
					}
					case MEMBER: {
						pANTLR3_BASE_TREE argNode = (pANTLR3_BASE_TREE) postfixNode->getChild(postfixNode, 0);
						unaryExpr->MemberStr = (char *) argNode->getText(argNode)->chars;

						unaryExpr->Type = IRUnaryOperator::Member;

						break;
					}
					case PTR: {
						pANTLR3_BASE_TREE argNode = (pANTLR3_BASE_TREE) postfixNode->getChild(postfixNode, 0);
						unaryExpr->MemberStr = (char *) argNode->getText(argNode)->chars;

						unaryExpr->Type = IRUnaryOperator::Ptr;

						break;
					}

					default:
						if (op == "++")
							unaryExpr->Type = IRUnaryOperator::Postincrement;
						else if (op == "--")
							unaryExpr->Type = IRUnaryOperator::Postdecrement;
						else
							Diag().Error("Unrecognized postfix operator " + op, DiagNode(CurrFilename, node));
				}

				if(unaryExpr->Type == IRUnaryOperator::Postincrement || unaryExpr->Type == IRUnaryOperator::Postdecrement) {
					if(dynamic_cast<IRVariableExpression*>(unaryExpr->BaseExpression) == nullptr) {
						Diag().Error("Operator can only be applied to a variable");
						return nullptr;
					}
				}

				unaryExpr->SetDiag(DiagNode(CurrFilename, postfixNode));

				innerExpression = unaryExpr;
			}

			return innerExpression;
		}
		case TERNARY: {
			assert(node->getChildCount(node) == 3);
			pANTLR3_BASE_TREE cond = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE left = (pANTLR3_BASE_TREE) node->getChild(node, 1);
			pANTLR3_BASE_TREE right = (pANTLR3_BASE_TREE) node->getChild(node, 2);

			IRExpression *condExpr = Parse_Expression(cond, containing_scope);
			IRExpression *leftExpr = Parse_Expression(left, containing_scope);
			IRExpression *rightExpr = Parse_Expression(right, containing_scope);

			if(condExpr == nullptr || leftExpr == nullptr || rightExpr == nullptr) {
				return nullptr;
			}

			IRTernaryExpression *expr = new IRTernaryExpression(containing_scope, condExpr, leftExpr, rightExpr);
			expr->SetDiag(DiagNode(CurrFilename, node));

			return expr;
		}
		default: {
			assert(node->getChildCount(node) == 2);
			pANTLR3_BASE_TREE left = (pANTLR3_BASE_TREE) node->getChild(node, 0);
			pANTLR3_BASE_TREE right = (pANTLR3_BASE_TREE) node->getChild(node, 1);

			IRExpression *leftExpr = Parse_Expression(left, containing_scope);
			IRExpression *rightExpr = Parse_Expression(right, containing_scope);

			if(leftExpr == nullptr || rightExpr == nullptr) {
				return nullptr;
			}

			// cannot assign from a definition
			if(dynamic_cast<IRDefineExpression*>(rightExpr)) {
				Diag().Error("Cannot assign from a declaration expression", rightExpr->Diag());
				return nullptr;
			}

			// binary operators
			std::string op = (char *) node->getText(node)->chars;

			IRBinaryExpression *exp = new IRBinaryExpression(containing_scope);
			exp->SetDiag(DiagNode(CurrFilename, node));
			exp->Left = leftExpr;
			exp->Right = rightExpr;

			if (op == "=")
				exp->Type = BinaryOperator::Set;
			else if (op == "+=")
				exp->Type = BinaryOperator::AddAndSet;
			else if (op == "-=")
				exp->Type = BinaryOperator::SubAndSet;
			else if (op == "/=")
				exp->Type = BinaryOperator::DivAndSet;
			else if (op == "*=")
				exp->Type = BinaryOperator::MulAndSet;
			else if (op == ">>=")
				exp->Type = BinaryOperator::RSAndSet;
			else if (op == "<<=")
				exp->Type = BinaryOperator::LSAndSet;
			else if (op == "|=")
				exp->Type = BinaryOperator::OrAndSet;
			else if (op == "^=")
				exp->Type = BinaryOperator::XOrAndSet;
			else if (op == "%=")
				exp->Type = BinaryOperator::ModAndSet;
			else if (op == "&=")
				exp->Type = BinaryOperator::AndAndSet;

			else if (op == "+")
				exp->Type = BinaryOperator::Add;
			else if (op == "-")
				exp->Type = BinaryOperator::Subtract;
			else if (op == "/")
				exp->Type = BinaryOperator::Divide;
			else if (op == "*")
				exp->Type = BinaryOperator::Multiply;
			else if (op == "%")
				exp->Type = BinaryOperator::Modulo;

			// We can't determine if this should be a signed shift yet,
			// since the type of the LHS might not be known (if it's
			// a variable expression we don't know it until the symbol
			// is resolved).
			else if (op == ">>")
				exp->Type = BinaryOperator::ShiftRight;

			else if (op == "<<")
				exp->Type = BinaryOperator::ShiftLeft;
			else if (op == ">>>")
				exp->Type = BinaryOperator::RotateRight;

			else if (op == "<<<")
				exp->Type = BinaryOperator::RotateLeft;

			else if (op == "|")
				exp->Type = BinaryOperator::Bitwise_Or;
			else if (op == "||")
				exp->Type = BinaryOperator::Logical_Or;
			else if (op == "&")
				exp->Type = BinaryOperator::Bitwise_And;
			else if (op == "&&")
				exp->Type = BinaryOperator::Logical_And;
			else if (op == "^")
				exp->Type = BinaryOperator::Bitwise_XOR;

			else if (op == "<")
				exp->Type = BinaryOperator::LessThan;
			else if (op == "<=")
				exp->Type = BinaryOperator::LessThanEqual;
			else if (op == ">")
				exp->Type = BinaryOperator::GreaterThan;
			else if (op == ">=")
				exp->Type = BinaryOperator::GreaterThanEqual;
			else if (op == "==")
				exp->Type = BinaryOperator::Equality;
			else if (op == "!=")
				exp->Type = BinaryOperator::Inequality;

			else
				Diag().Error("Unrecognized operator " + op, DiagNode(CurrFilename, node));


			// If we're a set operation, then the LHS must be settable
			if(BinaryOperator::IsAssignment(exp->Type)) {
				if(!CanAssign(exp->Left)) {
					Diag().Error("Expression cannot be assigned to.", exp->Left->Diag());
					return nullptr;
				}
			} else {
				// check to make sure we're not doing something crazy like adding to a definition
				// TODO: this should be a syntax error
				if(dynamic_cast<IRDefineExpression*>(exp->Left) || dynamic_cast<IRDefineExpression*>(exp->Right)) {
					Diag().Error("Cannot perform arithmetic on value of this type", DiagNode(CurrFilename, node));
					return nullptr;
				}
			}

			return exp;
		}

	}

}

IRBody *IRBody::CreateBodyWithScope(IRScope &scope)
{
	IRBody *body = new IRBody(*scope.ContainingScope, scope);
	return body;
}

IRIterationStatement *IRIterationStatement::CreateDoWhile(IRScope &scope, IRExpression &Expr, IRStatement &Body)
{
	IRIterationStatement *iter = new IRIterationStatement(scope);
	iter->Type = IRIterationStatement::ITERATE_DO_WHILE;
	iter->Expr = &Expr;
	iter->Body = &Body;

	return iter;
}

IRIterationStatement *IRIterationStatement::CreateWhile(IRScope &scope, IRExpression &Expr, IRStatement &Body)
{
	IRIterationStatement *iter = new IRIterationStatement(scope);
	iter->Type = IRIterationStatement::ITERATE_WHILE;
	iter->Expr = &Expr;
	iter->Body = &Body;

	return iter;
}

IRIterationStatement *IRIterationStatement::CreateFor(IRScope &scope, IRExpression &Start, IRExpression &Check, IRExpression &End, IRStatement &Body)
{
	IRIterationStatement *iter = new IRIterationStatement(scope);
	iter->Type = IRIterationStatement::ITERATE_FOR;
	iter->For_Expr_Start = &Start;
	iter->For_Expr_Check = &Check;
	iter->Expr = &End;
	iter->Body = &Body;
	return iter;
}
