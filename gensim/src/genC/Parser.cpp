/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <assert.h>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <math.h>
#include <string.h>
#include <fstream>

#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRRegisterExpression.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSAContext.h"
#include "genC/InstStructBuilder.h"

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"

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
			} else if(unary_expr->Type == IRUnaryOperator::Sequence) {
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

	// Then helper functions
	action = GetHelper(name);
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
	if (IntrinsicTable.find(name) != IntrinsicTable.end()) return IntrinsicTable.at(name);

	return nullptr;
}

bool GenCContext::AddFile(std::string filename)
{
	file_list.push_back(FileContents(filename, std::make_shared<std::ifstream>(filename.c_str())));
	return true;
}

bool GenCContext::AddFile(const FileContents &file)
{
	file_list.push_back(file);
	return true;
}

bool GenCContext::Parse()
{
	bool success = true;

	for (auto file : file_list) {
		CurrFilename = file.Filename;

		GenC::GenCScanner scanner(file.Stream.get());

		GenC::AstNode root(GenCNodeType::ROOT);
		GenC::GenCParser parser(scanner, &root);

		int result = parser.parse();

		if (result != 0) {
			Diag().Error("Syntax errors detected in ISA behaviour", DiagNode());
			return false;
		}

		success &= Parse_File(root);
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
	InsertConstant("NAN", IRTypes::Double, IRConstant::Double(__builtin_nanf("")));
	// ConstantTable["NAN"] = std::pair<IRSymbol*, double>(new IRSymbol("NAN", IRTypes::Double, Symbol_Constant, NULL), NAN);

	// load constants from arch
	for(auto &constant : Arch.GetConstants()) {
		// parse type
		if(!GetTypeManager()->HasNamedBasicType(constant.second.first)) {
			Diag().Error("Unknown type " + constant.second.first);
			continue;
		}
		IRType type = GetTypeManager()->GetBasicTypeByName(constant.second.first);

		IRConstant c;
		if(type.IsFloating()) {
			if(type == IRTypes::Double) {
				c = IRConstant::Double(strtod(constant.second.second.c_str(), nullptr));
			} else if(type == IRTypes::Float) {
				c = IRConstant::Float(strtod(constant.second.second.c_str(), nullptr));
			} else {
				UNEXPECTED;
			}
		} else {
			c = IRConstant::Integer(strtol(constant.second.second.c_str(), nullptr, 0));
		}

		InsertConstant(constant.first, type, c);
	}
}

void GenCContext::LoadRegisterNames()
{
	// load register file names
	int regbankid = 0;
	for (const auto &bank : Arch.GetRegFile().GetBanks()) {
		InsertConstant(bank->ID, IRTypes::UInt32, IRConstant::Integer(regbankid++));
	}

	// now load individual registers
	regbankid = 0;
	for (const auto &slot : Arch.GetRegFile().GetSlots()) {
		InsertConstant(slot->GetID(), IRTypes::UInt32, IRConstant::Integer(regbankid++));
	}

	// also load memory interface names
	regbankid = 0;
	for(const auto &interface : Arch.GetMemoryInterfaces().GetInterfaces()) {
		InsertConstant(interface.first, IRTypes::UInt32, IRConstant::Integer(interface.second.GetID()));
	}
}

void GenCContext::LoadFeatureNames()
{
	// load register file names
	int regbankid = 0;
	for (const auto &feature : Arch.GetFeatures()) {
		InsertConstant(feature.GetName(), IRTypes::UInt32, IRConstant::Integer(feature.GetId()));
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

	auto context = new ssa::SSAContext(ISA, Arch, type_manager_);

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


void GenCContext::BuildStructTypes()
{
	gensim::genc::InstStructBuilder isb;
	auto structType = isb.BuildType(&ISA, *type_manager_);
	GetTypeManager()->InsertStructType("Instruction", structType);

	gensim::genc::StructBuilder sb;
	for(auto &struct_type : ISA.UserStructTypes) {
		if(!GetTypeManager()->HasStructType(struct_type.GetName())) {
			GetTypeManager()->InsertStructType(struct_type.GetName(), sb.BuildStruct(&ISA, &struct_type, *type_manager_));
		}
	}
}

FileContents::FileContents(const std::string& filename, std::shared_ptr<std::istream> stream) : Filename(filename), Stream(stream)
{

}

GenCContext::GenCContext(const gensim::arch::ArchDescription &arch, const isa::ISADescription &isa, DiagnosticContext &diag_ctx) : Valid(true), Arch(arch), ISA(isa), GlobalScope(IRScope::CreateGlobalScope(*this)), diag_ctx(diag_ctx)
{
	type_manager_ = std::shared_ptr<ssa::SSATypeManager>(new ssa::SSATypeManager());

	for(auto name : arch.GetTypenames()) {
		type_manager_->InstallNamedType(name.first, type_manager_->GetBasicTypeByName(name.second));
	}

	// build the instruction structure
	BuildStructTypes();

	// Load register and register file names
	LoadRegisterNames();
	LoadFeatureNames();
	LoadStandardConstants();
	LoadIntrinsics();
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

void GenCContext::InsertConstant(std::string name, const IRType &type, IRConstant value)
{
	IRSymbol *sym = GlobalScope.InsertSymbol(name, type, Symbol_Constant);
	ConstantTable[name] = std::make_pair(sym, value);
}

std::pair<IRSymbol*, IRConstant> GenCContext::GetConstant(std::string name) const
{
	return ConstantTable.at(name);
}


bool GenCContext::Parse_File(GenC::AstNode &File)
{
	assert(File.GetType() == GenCNodeType::ROOT);

	// get definition list out of root node
	auto deflist = File.GetChildren().at(0);
	assert(deflist->GetType() == GenCNodeType::DefinitionList);

	bool success = true;
	for (auto child : *deflist) {
		switch (child->GetType()) {
			case GenCNodeType::Execute:
				success &= Parse_Execute(*child);
				break;
			case GenCNodeType::Helper:
				success &= Parse_Helper(*child);
				break;
			case GenCNodeType::Typename:
				success &= Parse_Typename(*child);
				break;
			case GenCNodeType::Constant:
				success &= Parse_Constant(*child);
				break;
			default:
				Diag().Error("Parse error: Expected helper, execute or behaviour node", DiagNode(CurrFilename));
				success = false;
				break;
		}
	}

	return success;
}

bool GenCContext::Parse_Execute(GenC::AstNode &Execute)
{
	assert(Execute.GetType() == GenCNodeType::Execute);
	assert(Execute.GetChildren().size() == 2);

	auto nameNode = Execute.GetChildren().at(0);
	auto bodyNode = Execute.GetChildren().at(1);

	std::string nameStr = nameNode->GetString();

	if (ExecuteTable.count(nameStr)) {
		diag_ctx.Error("Execute action " + nameStr + " redefined", DiagNode("", nameNode->GetLocation()));
		return false;
	}

	if(GetIntrinsic(nameStr) != nullptr) {
		diag_ctx.Error("The name " + nameStr + " is reserved by an intrinsic function", DiagNode("", nameNode->GetLocation()));
		return false;
	}

	IRExecuteAction *execute = new IRExecuteAction(nameStr, *this, GetTypeManager()->GetStructType("Instruction"));
	execute->SetDiag(DiagNode("", Execute.GetLocation()));

	IRBody *body = Parse_Body(*bodyNode, *execute->GetFunctionScope());
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
		Diag().Error(Format("Parse error in %s", nameStr.c_str()), DiagNode(CurrFilename, Execute.GetLocation()));
		return false;
	}

	ExecuteTable[execute->GetSignature().GetName()] = execute;
	return true;
}

bool GenCContext::Parse_Behaviour(GenC::AstNode &Action)
{
	// Do nothing since behaviour actions are handled elsewhere
	return true;
}

bool GenCContext::Parse_Constant(GenC::AstNode &Node)
{
	assert(Node.GetType() == GenCNodeType::Constant);
	assert(Node.GetChildren().size() == 2);
	bool success = true;

	GenC::AstNode *nameNode = Node.GetChildren().at(0);
	GenC::AstNode *typeNode = Node.GetChildren().at(1);
	GenC::AstNode *valueNode = Node.GetChildren().at(2);

	IRType type;
	success &= Parse_Type(*typeNode, type);
	type.Const = true;
	std::string id = nameNode->GetString();
	uint32_t constant = Parse_ConstantInt(*valueNode);

	InsertConstant(id, type, IRConstant::Integer(constant));

	return success;
}

bool GenCContext::Parse_Typename(GenC::AstNode &Node)
{
	assert(Node.GetType() == GenCNodeType::Typename);
	assert(Node.GetChildren().size() == 2);
	bool success = true;

	auto nameNode = Node.GetChildren().at(0);
	auto typeNode = Node.GetChildren().at(1);

	std::string name = nameNode->GetString();

	IRType target_type;
	success &= Parse_Type(*typeNode, target_type);

	if(GetTypeManager()->HasNamedBasicType(name) || GetTypeManager()->HasStructType(name)) {
		Diag().Error("Duplicate type name " + name + ".");
		return false;
	}
	GetTypeManager()->InstallNamedType(name, target_type);

	return success;
}


bool GenCContext::Parse_Helper(GenC::AstNode &node)
{
	assert(node.GetType() == GenCNodeType::Helper);

	auto scopeNode = node.GetChildren().at(0);
	auto typeNode = node.GetChildren().at(1);
	auto nameNode = node.GetChildren().at(2);
	auto paramsNode = node.GetChildren().at(3);
	auto attrsNode = node.GetChildren().at(4);
	auto bodyNode = node.GetChildren().at(5);

	std::string helpername = nameNode->GetString();
	if (HelperTable.count(helpername)) {
		diag_ctx.Error("Helper function " + helpername + " redefined", DiagNode("", nameNode->GetLocation()));
		return false;
	}

	std::vector<IRParam> param_list;

	for (uint32_t i = 0; i < paramsNode->GetChildren().size(); ++i) {
		auto paramNode = paramsNode->GetChildren().at(i);

		auto paramTypeNode = paramNode->GetChildren().at(0);
		auto paramNameNode = paramNode->GetChildren().at(1);

		std::string paramName = paramNameNode->GetString();

		IRType paramType;
		if(!Parse_Type(*paramTypeNode, paramType)) {
			return false;
		}

		if(!paramType.Reference && paramType.IsStruct()) {
			diag_ctx.Error("Helper function " + helpername + ", Parameter " + std::to_string(i+1) + ": cannot pass structs by value.");
			return false;
		}

		param_list.push_back(IRParam(paramName, paramType));
	}

	bool inlined = false;
	bool noinlined = false;
	bool exported = false;
	IRConstness::IRConstness helper_constness = IRConstness::never_const;
	for (uint32_t i = 0; i < attrsNode->GetChildren().size(); ++i) {
		auto attrNode = attrsNode->GetChildren().at(i);

		std::string attr_name = attrNode->GetString();

		if(attr_name == "inline") {
			inlined = true;
		} else if(attr_name == "noinline") {
			noinlined = true;
		} else if(attr_name == "export") {
			exported = true;
		} else if(attr_name == "global") {
			// Probably need to do something here?
		} else {
			diag_ctx.Warning("Unknown attribute: " + attr_name);
		}
	}

	HelperScope scope = InternalScope;
	std::string scopename = scopeNode->GetChildren().at(0)->GetString();
	if(scopename == "default") {
		scope = InternalScope;
	} else if(scopename == "internal") {
		scope = InternalScope;
	} else if(scopename == "private") {
		scope = PrivateScope;
	} else if(scopename == "public") {
		scope = PublicScope;
	}

	IRType return_type;
	if(!Parse_Type(*typeNode, return_type)) {
		return false;
	}
	if(return_type.Reference) {
		Diag().Error("Cannot return a reference from a helper function");
		return false;
	}

	std::string name = nameNode->GetString();
	IRSignature sig(name, return_type, param_list);
	if (noinlined) {
		sig.AddAttribute(ActionAttribute::NoInline);
	}
	if(exported) {
		sig.AddAttribute(ActionAttribute::Export);
	}
	sig.AddAttribute(ActionAttribute::Helper);

	IRHelperAction *helper = new IRHelperAction(sig, scope, *this);

	helper->body = Parse_Body(*bodyNode, *helper->GetFunctionScope());
	if (helper->body == NULL) return false;

	HelperTable[helper->GetSignature().GetName()] = helper;

	return true;
}

bool GenCContext::Parse_Type(GenC::AstNode &node, IRType &out_type)
{
	assert(node.GetType() == GenCNodeType::Type);

	bool success = true;
	out_type = GetTypeManager()->GetVoidType();

	auto rootNode = node.GetChildren().at(0);

	switch(rootNode->GetType()) {
		case GenCNodeType::Type: {
			auto type_name = rootNode->GetChildren().at(0)->GetString();
			if(GetTypeManager()->HasNamedBasicType(type_name)) {
				out_type = GetTypeManager()->GetBasicTypeByName(type_name);
			} else {
				Diag().Error("Unknown basic type " + type_name, DiagNode(CurrFilename, node.GetLocation()));
				return false;
			}
			break;
		}
		case GenCNodeType::Struct: {
			auto type_name = rootNode->GetChildren().at(0)->GetString();
			if(GetTypeManager()->HasStructType(type_name)) {
				out_type = GetTypeManager()->GetStructType(type_name);
			} else {
				Diag().Error("Unknown struct type " + type_name, DiagNode(CurrFilename, node.GetLocation()));
				return false;
			}
			break;
		}
		case GenCNodeType::Typename: {
			auto type_name = rootNode->GetChildren().at(0)->GetString();
			if(GetTypeManager()->HasNamedBasicType(type_name)) {
				out_type = GetTypeManager()->GetBasicTypeByName(type_name);
			} else {
				Diag().Error("Unknown typename " + type_name, DiagNode(CurrFilename, node.GetLocation()));
				return false;
			}
			break;
		}

		default: {
			UNEXPECTED;
		}
	}

	// parse annotations if they're there
	if(node.GetChildren().size() > 1) {
		auto annotationsList = node.GetChildren().at(1);
		assert(annotationsList->GetType() == GenCNodeType::Annotation);

		for(auto annotation : *annotationsList) {
			switch(annotation->GetType()) {
				case GenCNodeType::Vector:  {
					auto vectorwidth = annotation->GetChildren().at(0);
					out_type.VectorWidth = strtol(vectorwidth->GetString().c_str(), nullptr, 0);
					break;
				}
				case GenCNodeType::Reference:
					out_type.Reference = true;
					break;
				default:
					UNEXPECTED;
			}
		}
	}

	return true;
}

IRBody *GenCContext::Parse_Body(GenC::AstNode &node, IRScope &containing_scope, IRScope *override_scope)
{
	assert(node.GetType() == GenCNodeType::Body);
	IRBody *body;
	if (override_scope != NULL)
		body = IRBody::CreateBodyWithScope(*override_scope);
	else
		body = new IRBody(containing_scope);

	body->SetDiag(DiagNode(CurrFilename, node.GetLocation()));

	for (uint32_t i = 0; i < node.GetChildren().size(); ++i) {
		auto statementNode = node.GetChildren().at(i);
		IRStatement *statement = Parse_Statement(*statementNode, (body->Scope));
		if (statement == NULL) return NULL;

		body->Statements.push_back(statement);

		if (auto ret = dynamic_cast<IRFlowStatement*> (statement)) {
			if (ret->Type == IRFlowStatement::FLOW_RETURN_VALUE || ret->Type == IRFlowStatement::FLOW_RETURN_VOID) {
				if (i < (node.GetChildren().size() - 1)) {
					Diag().Error("Return statement should be at end of block", DiagNode(CurrFilename, node.GetLocation()));
					return NULL;
				}
			}
		}
	}

	return body;
}

IRStatement *GenCContext::Parse_Statement(GenC::AstNode &node, IRScope &containing_scope)
{
	auto node_location = node.GetLocation();

	switch (node.GetType()) {
		case GenCNodeType::Body: {
			return Parse_Body(node, containing_scope, NULL);
		}
		case GenCNodeType::Case: {
			if (containing_scope.Type != IRScope::SCOPE_SWITCH) {
				Diag().Error("Case not in switch statement", DiagNode(CurrFilename, node_location));
				break;
			}
			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node_location));
			c->Type = IRFlowStatement::FLOW_CASE;
			c->Expr = Parse_Expression(*node.GetChildren().at(0), containing_scope);
			if(c->Expr == nullptr) {
				return nullptr;
			}
			if(dynamic_cast<IRConstExpression*>(c->Expr) == nullptr) {
				Diag().Error("Case statement expression must be a constant\n", DiagNode(CurrFilename, node_location));
				return nullptr;
			}

			IRScope *CaseScope = new IRScope(containing_scope, IRScope::SCOPE_CASE);
			c->Body = Parse_Body(*node.GetChildren().at(1), containing_scope, CaseScope);
			if (c->Body == NULL) return NULL;
			return c;
		}
		case GenCNodeType::Default: {
			if (containing_scope.Type != IRScope::SCOPE_SWITCH) {
				Diag().Error("Default not in switch statement", DiagNode(CurrFilename, node_location));
				break;
			}

			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node_location));
			c->Type = IRFlowStatement::FLOW_DEFAULT;
			IRScope *CaseScope = new IRScope(containing_scope, IRScope::SCOPE_CASE);
			c->Body = Parse_Body(*node.GetChildren().at(0), containing_scope, CaseScope);
			if (c->Body == NULL) return NULL;
			return c;
		}
		case GenCNodeType::Break: {
			if (!containing_scope.ScopeBreakable()) {
				Diag().Error("Break not in breakable context", DiagNode(CurrFilename, node_location));
				break;
			}
			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node_location));
			c->Type = IRFlowStatement::FLOW_BREAK;
			return c;
		}
		case GenCNodeType::Continue: {
			if (!containing_scope.ScopeContinuable()) {
				Diag().Error("Continue not in continuable context", DiagNode(CurrFilename, node_location));
				break;
			}
			IRFlowStatement *c = new IRFlowStatement(containing_scope);
			c->SetDiag(DiagNode(CurrFilename, node_location));
			c->Type = IRFlowStatement::FLOW_CONTINUE;
			return c;
		}
		case GenCNodeType::Raise: {
			IRFlowStatement *ret = new IRFlowStatement(containing_scope);
			ret->SetDiag(DiagNode(CurrFilename, node_location));
			ret->Type = IRFlowStatement::FLOW_RAISE;

			return ret;
		}
		case GenCNodeType::Return: {
			IRFlowStatement *ret = new IRFlowStatement(containing_scope);
			ret->SetDiag(DiagNode(CurrFilename, node_location));

			if (node.GetChildren().size() != 0) {
				if(!containing_scope.GetContainingAction().GetSignature().HasReturnValue()) {
					Diag().Error("Tried to return a value from a function with a void return type", ret->Diag());
					return nullptr;
				}

				ret->Type = IRFlowStatement::FLOW_RETURN_VALUE;
				ret->Expr = Parse_Expression(*node.GetChildren().at(0), containing_scope);
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
		case GenCNodeType::While: {
			auto exprNode = node.GetChildren().at(0);
			auto bodyNode = node.GetChildren().at(1);

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_LOOP);
			auto expr = Parse_Expression(*exprNode, containing_scope);
			if(expr == nullptr) {
				return nullptr;
			}
			auto stmt = Parse_Statement(*bodyNode, containing_scope);
			if(stmt == nullptr) {
				return nullptr;
			}
			IRIterationStatement *iter = IRIterationStatement::CreateWhile(*scope, *expr, *stmt);
			iter->SetDiag(DiagNode(CurrFilename, node_location));
			return iter;
		}
		case GenCNodeType::Do: {
			auto exprNode = node.GetChildren().at(1);
			auto bodyNode = node.GetChildren().at(0);

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_LOOP);

			IRExpression *expr = Parse_Expression(*exprNode, *scope);
			if(expr == nullptr) {
				return nullptr;
			}
			IRStatement *body = Parse_Statement(*bodyNode, *scope);
			if(body == nullptr) {
				return nullptr;
			}

			IRIterationStatement *iter = IRIterationStatement::CreateDoWhile(*scope, *expr, *body);
			iter->SetDiag(DiagNode(CurrFilename, node_location));
			return iter;
		}
		case GenCNodeType::For: {
			IRExpression *start = nullptr;
			IRExpression *check = nullptr;
			IRExpression *post = nullptr;

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_LOOP);

			auto for_pre = node.GetChildren().at(0);
			auto for_check = node.GetChildren().at(1);
			auto for_post = node.GetChildren().at(2);

			// for_ptr and for_post are expression statements, not expressions, so dereference them to expressions
			for_pre = for_pre->GetChildren().at(0);
			for_post = for_post->GetChildren().at(0);

			start = Parse_Expression(*for_pre, *scope);
			check = Parse_Expression(*for_check, *scope);
			post  = Parse_Expression(*for_post, *scope);

			IRExpression *post_expression = post;
			if(start == nullptr) {
				start = new IRConstExpression(*scope, IRTypes::UInt8, IRConstant::Integer(0));
			}
			if(check == nullptr) {
				check = new IRConstExpression(*scope, IRTypes::UInt8, IRConstant::Integer(0));
			}
			if(post == nullptr) {
				post = new IRConstExpression(*scope, IRTypes::UInt8, IRConstant::Integer(0));
			}

			auto bodyNode = node.GetChildren().at(3);
			GASSERT(bodyNode != nullptr);

			IRIterationStatement *iter = IRIterationStatement::CreateFor(*scope, *start, *check, *post_expression, *Parse_Statement(*bodyNode, *scope));
			iter->SetDiag(DiagNode(CurrFilename, node_location));
			return iter;
		}
		case GenCNodeType::If: {
			auto exprNode = node.GetChildren().at(0);
			auto thenNode = node.GetChildren().at(1);

			IRSelectionStatement *stmt;

			// exprnode is an expression statement so dereference it
			exprNode = exprNode->GetChildren().at(0);

			auto expr = Parse_Expression(*exprNode, containing_scope);
			auto then_stmt = Parse_Statement(*thenNode, containing_scope);

			if(dynamic_cast<IRDefineExpression*>(expr)) {
				Diag().Error("Cannot define variable in if statement expression", DiagNode(CurrFilename, node_location));
				return nullptr;
			}

			if (node.GetChildren().size() == 3) {
				auto elseNode = node.GetChildren().at(2);
				auto else_stmt = Parse_Statement(*elseNode, containing_scope);
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

			stmt->SetDiag(DiagNode(CurrFilename, exprNode->GetLocation()));
			return stmt;
		}
		case GenCNodeType::Switch: {
			auto exprNode = node.GetChildren().at(0);
			auto bodyNode = node.GetChildren().at(1);

			IRScope *scope = new IRScope(containing_scope, IRScope::SCOPE_SWITCH);

			auto expr = Parse_Expression(*exprNode, containing_scope);
			auto body = Parse_Body(*bodyNode, containing_scope, scope);

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
			sel->SetDiag(DiagNode(CurrFilename, exprNode->GetLocation()));

			return sel;
		}
		case GenCNodeType::ExprStmt: {
			auto exprNode = node.GetChildren().at(0);

			auto expression = Parse_Expression(*exprNode, containing_scope);
			if(expression == nullptr) {
				return nullptr;
			}

			IRExpressionStatement *expr = new IRExpressionStatement(containing_scope, *expression);
			expr->SetDiag(DiagNode(CurrFilename, node_location));
			return expr;
		}
		default: {
			throw std::logic_error("Unknown node type");
		}
	}
	return NULL;
}

uint64_t GenCContext::Parse_ConstantInt(GenC::AstNode &node)
{
	return strtol(node.GetChildren().at(0)->GetString().c_str(), nullptr, 0);
}

static bool can_be_int32(uint64_t v)
{
	return v <= std::numeric_limits<int32_t>::max();
}

static bool can_be_int64(uint64_t v)
{
	return v <= std::numeric_limits<int64_t>::max();
}

IRExpression *GenCContext::Parse_Expression(GenC::AstNode &node, IRScope &containing_scope)
{
	auto node_location = node.GetLocation();

	switch (node.GetType()) {
		// BODY node type represents empty expression
		case GenCNodeType::Body: {
			assert(node.GetChildren().size() == 0);
			EmptyExpression *expr = new EmptyExpression(containing_scope);
			expr->SetDiag(DiagNode(CurrFilename, node_location));
			return expr;
		}
		case GenCNodeType::INT: {
			// final type of constant depends on if it has any type length specifiers
			bool unsigned_specified = false;
			bool long_specified = false;
			bool is_signed = true;
			bool is_long = false;

			std::string text = node.GetChildren().at(0)->GetString();
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
				Diag().Warning("Constant value truncated", DiagNode(CurrFilename, node_location));
			}
			auto expr = new IRConstExpression(containing_scope, type, iv);
			expr->SetDiag(DiagNode(CurrFilename, node_location));
			return expr;

		}

		case GenCNodeType::FLOAT: {
			auto valNode = node.GetChildren().at(0);

			IRConstant iv = IRConstant::Float(strtof(valNode->GetString().c_str(), nullptr));
			auto expr = new IRConstExpression(containing_scope, IRTypes::Float, iv);
			expr->SetDiag(DiagNode(CurrFilename, node_location));
			return expr;
		}

		case GenCNodeType::DOUBLE: {
			auto valNode = node.GetChildren().at(0);

			IRConstant iv = IRConstant::Double(strtod(valNode->GetString().c_str(), nullptr));
			auto expr = new IRConstExpression(containing_scope, IRTypes::Double, iv);
			expr->SetDiag(DiagNode(CurrFilename, node_location));
			return expr;
		}

		case GenCNodeType::Variable: {
			assert(node.GetChildren().size() == 1);

			auto nameNode = node.GetChildren().at(0);
			std::string id = nameNode->GetString();

			IRVariableExpression *expr = new IRVariableExpression(id, containing_scope);
			expr->SetDiag(DiagNode(CurrFilename, node_location));

			return expr;
		}
		case GenCNodeType::Declare: {
			assert(node.GetChildren().size() == 2);
			auto typeNode = node.GetChildren().at(0);
			auto nameNode = node.GetChildren().at(1);

			IRType type;
			if(!Parse_Type(*typeNode, type)) {
				return nullptr;
			}

			std::string name = nameNode->GetString();

			IRDefineExpression *expr = new IRDefineExpression(containing_scope, type, name);
			expr->SetDiag(DiagNode(CurrFilename, node_location));

			return expr;
		}

		case GenCNodeType::Call: {
			auto nameNode = node.GetChildren().at(0);

			IRCallExpression *call;

			// TODO: these should be handled as intrinsics
			std::string targetName = nameNode->GetString();
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

			call->SetDiag(DiagNode(CurrFilename, node_location));
			call->TargetName = targetName;

			auto paramListNode = node.GetChildren().at(1);

			bool success = true;
			for (uint32_t i = 0; i < paramListNode->GetChildren().size(); ++i) {
				auto argNode = paramListNode->GetChildren().at(i);
				IRExpression *argexpr = Parse_Expression(*argNode, containing_scope);
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

		case GenCNodeType::BitCast: {
			auto typeNode = node.GetChildren().at(0);
			auto innerNode = node.GetChildren().at(1);

			IRExpression *innerExpr = Parse_Expression(*innerNode, containing_scope);
			if(innerExpr == nullptr) {
				return nullptr;
			}

			//TODO: this should be a syntax error
			if(dynamic_cast<IRDefineExpression*>(innerExpr)) {
				Diag().Error("Cannot cast a definition", DiagNode(CurrFilename, node_location));
				return nullptr;
			}

			IRType target_type;
			if(!Parse_Type(*typeNode, target_type)) {
				return nullptr;
			}

			IRCastExpression *gce = new IRCastExpression(containing_scope, target_type, IRCastExpression::Bitcast);
			gce->SetDiag(DiagNode(CurrFilename, node_location));
			gce->Expr = innerExpr;

			return gce;
		}
		case GenCNodeType::Cast: {
			auto typeNode = node.GetChildren().at(0);
			auto innerNode = node.GetChildren().at(1);

			IRExpression *innerExpr = Parse_Expression(*innerNode, containing_scope);
			if(innerExpr == nullptr) {
				return nullptr;
			}

			//TODO: this should be a syntax error
			if(dynamic_cast<IRDefineExpression*>(innerExpr)) {
				Diag().Error("Cannot cast a definition", DiagNode(CurrFilename, node_location));
				return nullptr;
			}

			IRType target_type;
			if(!Parse_Type(*typeNode, target_type)) {
				return nullptr;
			}

			IRCastExpression *gce = new IRCastExpression(containing_scope, target_type);
			gce->SetDiag(DiagNode(CurrFilename, node_location));
			gce->Expr = innerExpr;

			return gce;
		}

		case GenCNodeType::Prefix: {
			assert(node.GetChildren().size() == 2);

			auto typeNode = node.GetChildren().at(0);
			auto innerNode = node.GetChildren().at(1);

			IRUnaryExpression *unary = new IRUnaryExpression(containing_scope);

			std::string typeString = typeNode->GetString();
			unary->BaseExpression = Parse_Expression(*innerNode, containing_scope);
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
				Diag().Error("Unrecognized prefix operator " + typeString, DiagNode(CurrFilename, node_location));
				return nullptr;
			}

			if(unary->Type == IRUnaryOperator::Preincrement || unary->Type == IRUnaryOperator::Predecrement) {
				if(dynamic_cast<IRVariableExpression*>(unary->BaseExpression) == nullptr) {
					Diag().Error("Operator can only be applied to a variable");
					return nullptr;
				}
			}

			unary->SetDiag(DiagNode(CurrFilename, node_location));
			return unary;
		}
		case GenCNodeType::Postfix: {
			auto opNode = node.GetChildren().at(0);
			auto innerNode = node.GetChildren().at(1);

			IRExpression *innerExpression = Parse_Expression(*innerNode, containing_scope);
			if(innerExpression == nullptr) {
				return nullptr;
			}

			IRUnaryExpression *unaryExpr = new IRUnaryExpression(containing_scope);
			unaryExpr->BaseExpression = innerExpression;

			switch (opNode->GetType()) {
				case GenCNodeType::VectorElement: {
					auto index_expression = opNode->GetChildren().at(0);

					unaryExpr->Type = IRUnaryOperator::Index;
					unaryExpr->Arg = Parse_Expression(*index_expression, containing_scope);

					break;
				}
				case GenCNodeType::BitExtract: {
					auto fromNode = opNode->GetChildren().at(0);
					auto toNode = opNode->GetChildren().at(1);

					unaryExpr->Type = IRUnaryOperator::Sequence;
					unaryExpr->Arg = new IRConstExpression(containing_scope, IRTypes::Int32, IRConstant::Integer(Parse_ConstantInt(*fromNode)));
					unaryExpr->Arg2 = new IRConstExpression(containing_scope, IRTypes::Int32, IRConstant::Integer(Parse_ConstantInt(*toNode)));
					break;
				}
				case GenCNodeType::Member: {
					auto memberNode = opNode->GetChildren().at(0);
					unaryExpr->MemberStr = memberNode->GetString();

					unaryExpr->Type = IRUnaryOperator::Member;

					break;
				}

				case GenCNodeType::STRING: {
					// try and
					auto opName = opNode->GetString();
					if (opName == "++")
						unaryExpr->Type = IRUnaryOperator::Postincrement;
					else if (opName == "--")
						unaryExpr->Type = IRUnaryOperator::Postdecrement;
					else
						Diag().Error("Unrecognized postfix operator " + opName, DiagNode(CurrFilename, node_location));
					break;
				}

				default: {
					UNEXPECTED;
				}
			}

			if(unaryExpr->Type == IRUnaryOperator::Postincrement || unaryExpr->Type == IRUnaryOperator::Postdecrement) {
				if(dynamic_cast<IRVariableExpression*>(unaryExpr->BaseExpression) == nullptr) {
					Diag().Error("Operator can only be applied to a variable");
					return nullptr;
				}
			}

			unaryExpr->SetDiag(DiagNode(CurrFilename, node.GetLocation()));

			return unaryExpr;
		}
		case GenCNodeType::Ternary: {
			auto condNode = node.GetChildren().at(0);
			auto leftNode = node.GetChildren().at(1);
			auto rightNode = node.GetChildren().at(2);

			IRExpression *condExpr = Parse_Expression(*condNode, containing_scope);
			IRExpression *leftExpr = Parse_Expression(*leftNode, containing_scope);
			IRExpression *rightExpr = Parse_Expression(*rightNode, containing_scope);

			if(condExpr == nullptr || leftExpr == nullptr || rightExpr == nullptr) {
				return nullptr;
			}

			IRTernaryExpression *expr = new IRTernaryExpression(containing_scope, condExpr, leftExpr, rightExpr);
			expr->SetDiag(DiagNode(CurrFilename, node_location));

			return expr;
		}
		case GenCNodeType::Vector: {
			std::vector<IRExpression *> elements;

			for(auto child : node.GetChildren()) {

				auto element = Parse_Expression(*child, containing_scope);
				elements.push_back(element);
			}

			IRVectorExpression *vector = new IRVectorExpression(containing_scope, elements);
			return vector;
		}

		case GenCNodeType::Binary: {
			auto opNode = node.GetChildren().at(0);
			auto leftNode = node.GetChildren().at(1);
			auto rightNode = node.GetChildren().at(2);

			IRExpression *leftExpr = Parse_Expression(*leftNode, containing_scope);
			IRExpression *rightExpr = Parse_Expression(*rightNode, containing_scope);

			if(leftExpr == nullptr || rightExpr == nullptr) {
				return nullptr;
			}

			// cannot assign from a definition
			if(dynamic_cast<IRDefineExpression*>(rightExpr)) {
				Diag().Error("Cannot assign from a declaration expression", rightExpr->Diag());
				return nullptr;
			}

			// binary operators
			std::string op = opNode->GetString();

			IRBinaryExpression *exp = new IRBinaryExpression(containing_scope);
			exp->SetDiag(DiagNode(CurrFilename, node_location));
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
			else if(op == "::") {
				exp->Type = BinaryOperator::VConcatenate;
			} else {
				Diag().Error("Unrecognized operator " + op, DiagNode(CurrFilename, node_location));
				return nullptr;
			}

			// If we're a RMW, then the LHS cannot be a declaration
			if(BinaryOperator::IsRMW(exp->Type)) {
				if(dynamic_cast<IRDefineExpression*>(exp->Left) != nullptr) {
					Diag().Error("Cannot have declaration on left hand side of RMW operator", exp->Diag());
					return nullptr;
				}
			}

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
					Diag().Error("Cannot perform arithmetic on value of this type", DiagNode(CurrFilename, node_location));
					return nullptr;
				}
			}

			return exp;
		}
		default: {
			UNEXPECTED;
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
