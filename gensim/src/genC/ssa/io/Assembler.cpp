/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"

#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "define.h"

#include <string>
#include <cstring>
#include <stdexcept>

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::io;

void AssemblyContext::ProcessPhiStatements()
{
	for(auto &phi : phi_members_) {
		for(auto &member : phi.second) {
			phi.first->Add((SSAStatement*)Get(member));
		}
	}
}

void AssemblyContext::AddPhiPlaceholder(SSAPhiStatement* stmt, const std::string& placeholder)
{
	phi_members_[stmt].insert(placeholder);
}

using gensim::genc::IRConstant;
static IRConstant parse_constant_value(const GenCSSA::AstNode &tree)
{
	switch(tree.GetType()) {
		case GenCSSANodeType::INT:
			return IRConstant::Integer(tree.GetInt());
		case GenCSSANodeType::FLOAT:
			return IRConstant::Float(tree.GetFloat());
		case GenCSSANodeType::DOUBLE:
			return IRConstant::Double(tree.GetDouble());
		default:
			UNREACHABLE;
	}


}

static SSAType parse_type(SSAContext &ctx, const GenCSSA::AstNode &tree)
{
	gensim::genc::IRType out;

	switch(tree.GetType()) {
		case GenCSSANodeType::BasicType:
			if(!gensim::genc::IRType::ParseType(tree[0].GetString(), out)) {
				throw std::logic_error("Unknown type");
			}
			break;
		case GenCSSANodeType::StructType:
			if(!ctx.GetTypeManager().HasStructType(tree[0].GetString())) {
				throw std::logic_error("Unknown type");
			} else {
				out = ctx.GetTypeManager().GetStructType(tree[0].GetString());
			}
			break;
		case GenCSSANodeType::VectorType:
			if(!gensim::genc::IRType::ParseType(tree[0].GetString(), out)) {
				throw std::logic_error("Unknown type");
			}
			out.VectorWidth = tree[1].GetInt();
			break;
		case GenCSSANodeType::ReferenceType:
			out = parse_type(ctx, tree[0]);
			out.Reference = true;
			break;
		default:
			UNEXPECTED;
	}

	return out;
}


void ContextAssembler::SetTarget(SSAContext* target)
{
	target_ = target;
}

bool ContextAssembler::Assemble(AssemblyFileContext &afc, DiagnosticContext &dc)
{
	auto tree = afc.GetTree()[0];
	if(tree.GetType() != GenCSSANodeType::Context) {
		throw std::logic_error("");
	}

	try {
		ActionAssembler actionasm;
		for(auto childPtr : tree[0]) {
			auto &child = *childPtr;
			SSAActionBase *action = nullptr;

			switch(child.GetType()) {
				case GenCSSANodeType::Action:
					action = actionasm.Assemble(child, *target_);
					break;
				default:
					UNEXPECTED;
			}
			target_->AddAction(action);
		}
	} catch(gensim::Exception &e) {
		dc.Error(e.what());
		return false;
	} catch (std::exception &e) {
		dc.Error(e.what());
		return false;
	}

	return true;
}

SSAFormAction* ActionAssembler::Assemble(const GenCSSA::AstNode &tree, SSAContext &context)
{
	if(tree.GetType() != GenCSSANodeType::Action) {
		throw std::logic_error("");
	}
	AssemblyContext ctx;
	auto action_id_node = tree[1];
	std::string action_id = action_id_node.GetString();

	auto &action_rtype_node = tree[0];
	SSAType return_type = parse_type(context, action_rtype_node);

	auto &attribute_list_node = tree[2];

	IRSignature::param_type_list_t params;
	auto &action_params_node = tree[3];
	for(auto &paramPtr : action_params_node) {
		auto &paramNode = *paramPtr;
		auto &type_node = paramNode[0];
		auto &name_node = paramNode[1];
		auto name_string = name_node.GetString();;

		IRParam param (name_string, parse_type(context, type_node));
		params.push_back(param);
	}

	IRSignature signature (action_id, return_type, params);

	for(auto attributeNodePtr : attribute_list_node) {
		auto &attributeNode = *attributeNodePtr;

		switch(attributeNode.GetType()) {
			case GenCSSANodeType::AttributeNoinline:
				signature.AddAttribute(ActionAttribute::NoInline);
				break;
			case GenCSSANodeType::AttributeHelper:
				signature.AddAttribute(ActionAttribute::Helper);
				break;
			default:
				UNEXPECTED;
		}
	}

	SSAActionPrototype prototype (signature);
	SSAFormAction *action = new SSAFormAction(context, prototype);
	action->Arch = &context.GetArchDescription();
	action->Isa = &context.GetIsaDescription();

	// map parameter names to params
	for(unsigned paramIdx = 0; paramIdx < action_params_node.GetChildren().size(); ++paramIdx) {
		auto &paramNode = action_params_node[paramIdx];

		auto name_node = paramNode[1];
		auto name_string = name_node.GetString();

		action->ParamSymbols.at(paramIdx)->SetName(name_string);
		ctx.Put(name_string, action->ParamSymbols.at(paramIdx));
	}

	auto &action_syms_node = tree[4];
	// 2 nodes per symbol: type, name
	for(auto actionSymPtr : action_syms_node) {
		auto &actionSymNode = *actionSymPtr;
		auto &type_node = actionSymNode[0];
		auto &name_node = actionSymNode[1];
		auto name_string = name_node.GetString();

		SSASymbol *sym = new SSASymbol(context, name_string, parse_type(context, type_node), Symbol_Local);
		sym->SetName(name_string);
		action->AddSymbol(sym);
		ctx.Put(name_string, sym);
	}

	// Parse blocks
	auto &block_def_list_node = tree[5];
	for(unsigned i = 0; i < block_def_list_node.GetChildren().size(); ++i) {
		auto &block_node = block_def_list_node[i];
		SSABlock *block = new SSABlock(action->GetContext(), *action);
		ctx.Put(block_node.GetString(), block);

		if(i == 0) {
			action->EntryBlock = block;
		}
	}

	auto &block_list_node = tree[6];
	BlockAssembler ba (ctx);
	for(auto blockNode : block_list_node) {
		ba.Assemble(*blockNode, *action);
	}

	ctx.ProcessPhiStatements();

	return action;
}

BlockAssembler::BlockAssembler(AssemblyContext& ctx) : ctx_(ctx)
{

}


SSABlock* BlockAssembler::Assemble(const GenCSSA::AstNode &tree, SSAFormAction &action)
{
	if(tree.GetType() != GenCSSANodeType::Block) {
		throw std::logic_error("");
	}

	// parse block name
	auto &block_name_node = tree[0];
	std::string block_name = block_name_node.GetString();

	SSABlock *block = dynamic_cast<SSABlock*>(ctx_.Get(block_name));
	if(block == nullptr) {
		throw std::logic_error("");
	}

	auto &block_statement_list = tree[1];

	StatementAssembler sa (action, ctx_);
	for(auto childPtr : block_statement_list) {
		sa.Assemble(*childPtr, block);
	}

	return block;
}

static gensim::genc::BinaryOperator::EBinaryOperator parse_binop(const GenCSSA::AstNode &tree)
{
	using namespace gensim::genc::BinaryOperator;
	return gensim::genc::BinaryOperator::ParseOperator(tree.GetString());
}

StatementAssembler::StatementAssembler(SSAFormAction& action, AssemblyContext &ctx) : action_(action), ctx_(ctx)
{

}

SSAStatement *StatementAssembler::parse_bank_reg_read_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &bank_node = tree[0];
	auto &index_node = tree[1];

	IRConstant bank = parse_constant_value(bank_node);
	SSAStatement *idx = get_statement(index_node);

	return SSARegisterStatement::CreateBankedRead(block, bank.Int(), idx);
}

SSAStatement *StatementAssembler::parse_bank_reg_write_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &bank_node = tree[0];
	auto &index_node = tree[1];
	auto &value_node = tree[2];

	IRConstant bank = parse_constant_value(bank_node);
	SSAStatement *idx = get_statement(index_node);
	SSAStatement *value = get_statement(value_node);

	return SSARegisterStatement::CreateBankedWrite(block, bank.Int(), idx, value);
}

SSAStatement *StatementAssembler::parse_binary_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &op_node = tree[0];
	auto &lhs_node = tree[1];
	auto &rhs_node = tree[2];

	auto lhs_statement = get_statement(lhs_node);
	auto rhs_statement = get_statement(rhs_node);

	auto statement_type = parse_binop(op_node);
	if(statement_type == BinaryOperator::ShiftRight && lhs_statement->GetType().Signed) {
		statement_type = BinaryOperator::SignedShiftRight;
	}

	return new SSABinaryArithmeticStatement(block, lhs_statement, rhs_statement, statement_type);
}

SSAStatement *StatementAssembler::parse_call_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto target_node = tree[0];
	std::string target_name = target_node.GetString();
	SSAActionBase *target = action_.GetContext().GetAction(target_name);

	std::vector<SSAValue*> params;
	auto &paramNode = target_node[1];
	for(unsigned i = 0; i < paramNode.GetChildren().size(); ++i) {
		auto &param = target->GetPrototype().ParameterTypes().at(i);
		if(param.Reference) {
			params.push_back(get_symbol(paramNode[i]));
		} else {
			params.push_back(get_statement(paramNode[i]));
		}
	}

	return new SSACallStatement(block, (SSAFormAction*)target, params);
}

SSACastStatement::CastType parse_cast_type(const GenCSSA::AstNode &node)
{
	std::string node_text = node.GetString();

	if(node_text == "zextend") {
		return SSACastStatement::Cast_ZeroExtend;
	} else if(node_text == "sextend") {
		return SSACastStatement::Cast_SignExtend;
	} else if(node_text == "truncate") {
		return SSACastStatement::Cast_Truncate;
	} else if(node_text == "convert") {
		return SSACastStatement::Cast_Convert;
	} else if(node_text == "splat") {
		return SSACastStatement::Cast_VectorSplat;
	} else if(node_text == "reinterpret") {
		return SSACastStatement::Cast_Reinterpret;
	} else {
		UNEXPECTED;
	}
}

SSACastStatement::CastOption parse_cast_option(const GenCSSA::AstNode &node)
{
	std::string node_text = node.GetString();

	if(node_text == "none") {
		return SSACastStatement::Option_None;
	} else if(node_text == "round_default") {
		return SSACastStatement::Option_RoundDefault;
	} else if(node_text == "round_ninfinity") {
		return SSACastStatement::Option_RoundTowardNInfinity;
	} else if(node_text == "round_pinfinity") {
		return SSACastStatement::Option_RoundTowardPInfinity;
	} else if(node_text == "round_nearest") {
		return SSACastStatement::Option_RoundTowardNearest;
	} else if(node_text == "round_zero") {
		return SSACastStatement::Option_RoundTowardZero;
	} else {
		UNEXPECTED;
	}
}

SSAStatement *StatementAssembler::parse_cast_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &cast_type_node = tree[0];
	auto &option_node = tree[1];
	auto &type_node = tree[2];
	auto &expr_node = tree[3];

	SSACastStatement::CastType ctype = parse_cast_type(cast_type_node);

	SSACastStatement::CastOption option = parse_cast_option(option_node);

	auto target_type = parse_type(action_.GetContext(), type_node);
	auto statement = get_statement(expr_node);

	auto cast_statement = new SSACastStatement(block, target_type, statement, ctype);
	cast_statement->SetOption(option);
	return cast_statement;
}

SSAStatement *StatementAssembler::parse_constant_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &type_node = tree[0];
	auto &value_node = tree[1];

	IRConstant constant_value = parse_constant_value(value_node);
	SSAType constant_type = parse_type(action_.GetContext(), type_node);

	if(SSAType::Cast(constant_value, constant_type, constant_type) != constant_value) {
		throw std::logic_error("Constant type cannot contain specified value");
	}

	return new SSAConstantStatement(block, constant_value, constant_type);
}

SSAStatement *StatementAssembler::parse_devread_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &dev_node = tree[0];
	auto &addr_node = tree[1];
	auto &target_node = tree[2];

	return new SSADeviceReadStatement(block, get_statement(dev_node), get_statement(addr_node), get_symbol(target_node));
}

SSAStatement *StatementAssembler::parse_devwrite_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	UNEXPECTED; // this is an intrinsic now
}

SSAStatement *StatementAssembler::parse_if_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &expr_node = tree[0];
	auto &true_node = tree[1];
	auto &false_node = tree[2];

	return new SSAIfStatement(block, get_statement(expr_node), *get_block(true_node), *get_block(false_node));
}

SSAStatement *StatementAssembler::parse_intrinsic_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &id_node = tree[0];
	uint32_t id = parse_constant_value(id_node).Int();

	UNIMPLEMENTED;

	/*SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(block, (gensim::genc::ssa::SSAIntrinsicStatement::IntrinsicType)id);

	auto &paramListNode = tree[1];
	for(auto paramNode : paramListNode) {
		intrinsic->AddArg(get_statement(*paramNode));
	}

	return intrinsic;*/
}

SSAStatement *StatementAssembler::parse_jump_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &target_name_node = tree[0];

	SSABlock *target = get_block(target_name_node);

	return new SSAJumpStatement(block, *target);
}

SSAStatement *StatementAssembler::parse_mem_read_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &width_node = tree[0];
	auto &addr_node = tree[1];
	auto &target_name_node = tree[2];

	IRConstant width = parse_constant_value(width_node);
	SSAStatement *addr = get_statement(addr_node);
	SSASymbol *target = get_symbol(target_name_node);

	gensim::arch::MemoryInterfaceDescription *interface = nullptr;

	return &SSAMemoryReadStatement::CreateRead(block, addr, target, width.Int(), false, interface);
}

SSAStatement *StatementAssembler::parse_mem_write_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &width_node = tree[0];
	auto &addr_node = tree[1];
	auto &value_node = tree[2];

	IRConstant width = parse_constant_value(width_node);
	SSAStatement *addr = get_statement(addr_node);
	SSAStatement *target = get_statement(value_node);

	gensim::arch::MemoryInterfaceDescription *interface = nullptr;

	return &SSAMemoryWriteStatement::CreateWrite(block, addr, target, width.Int(), interface);
}

SSAStatement *StatementAssembler::parse_read_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &target_name_node = tree[0];
	return new SSAVariableReadStatement(block, get_symbol(target_name_node));
}


SSAStatement *StatementAssembler::parse_reg_read_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &target_reg_node = tree[0];
	auto target_regid = parse_constant_value(target_reg_node);

	return SSARegisterStatement::CreateRead(block, target_regid.Int());
}

SSAStatement *StatementAssembler::parse_reg_write_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &target_reg_node = tree[0];
	auto target_regid = parse_constant_value(target_reg_node);
	auto &value_node = tree[1];

	return SSARegisterStatement::CreateWrite(block, target_regid.Int(), get_statement(value_node));
}

SSAStatement *StatementAssembler::parse_raise_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	return new SSARaiseStatement(block, nullptr);
}

SSAStatement *StatementAssembler::parse_return_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	if(tree.GetType() == GenCSSANodeType::StatementReturn) {
		auto &target_name_node = tree[0];
		return new SSAReturnStatement(block, get_statement(target_name_node));
	} else if(tree.GetType() == GenCSSANodeType::StatementReturnVoid) {
		return new SSAReturnStatement(block, nullptr);
	} else {
		UNEXPECTED;
	}
}

SSAStatement *StatementAssembler::parse_select_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &expr_node = tree[0];
	auto &true_node = tree[1];
	auto &false_node = tree[2];

	return new SSASelectStatement(block, get_statement(expr_node), get_statement(true_node), get_statement(false_node));
}

SSAStatement *StatementAssembler::parse_struct_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto struct_var_node = tree[0];
	auto member_node = tree[1];
	auto member_name = member_node.GetString();

	auto struct_symbol = get_symbol(struct_var_node);
	GASSERT(struct_symbol->GetType().IsStruct());
	GASSERT(struct_symbol->GetType().BaseType.StructType->HasMember(member_name));

	return new SSAReadStructMemberStatement(block, get_symbol(struct_var_node), {member_name});
}

SSAStatement *StatementAssembler::parse_switch_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto expr_node = tree[0];
	auto default_node = tree[1];

	SSASwitchStatement *stmt = new SSASwitchStatement(block, get_statement(expr_node), get_block(default_node));

	auto &listNode = tree[2];
	for(auto switchEntryPtr : listNode) {
		auto &switchEntry = *switchEntryPtr;
		auto value_node = switchEntry[0];
		auto target_node = switchEntry[1];

		stmt->AddValue(get_statement(value_node), get_block(target_node));
	}

	return stmt;
}

SSAStatement *StatementAssembler::parse_unop_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto op_node = tree[0];
	auto expr_node = tree[1];

	SSAUnaryOperator::ESSAUnaryOperator op_type;

	if(op_node.GetString() == "!") {
		op_type = SSAUnaryOperator::OP_NEGATE;
	} else if(op_node.GetString() == "~") {
		op_type = SSAUnaryOperator::OP_COMPLEMENT;
	} else if(op_node.GetString() == "-") {
		op_type = SSAUnaryOperator::OP_NEGATIVE;
	}  else {
		UNEXPECTED;
	}

	return new SSAUnaryArithmeticStatement(block, get_statement(expr_node), op_type);
}

SSAStatement *StatementAssembler::parse_vextract_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &base_node = tree[0];
	auto &index_node = tree[1];

	return new SSAVectorExtractElementStatement(block, get_statement(base_node), get_statement(index_node));
}

SSAStatement *StatementAssembler::parse_vinsert_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &base_node =  tree[0];
	auto &index_node = tree[1];
	auto &value_node = tree[2];

	return new SSAVectorInsertElementStatement(block, get_statement(base_node), get_statement(index_node), get_statement(value_node));
}

SSAStatement *StatementAssembler::parse_write_statement(const GenCSSA::AstNode &tree, SSABlock *block)
{
	auto &target_node = tree[0];
	auto target = get_symbol(target_node);

	auto &value_node = tree[1];
	auto value = get_statement(value_node);

	return new SSAVariableWriteStatement(block, target, value);
}

SSAStatement* StatementAssembler::parse_phi_statement(const GenCSSA::AstNode &tree, SSABlock* block)
{
	auto type_node = tree[0];
	auto type = parse_type(block->GetContext(), type_node);

	auto phistatement = new SSAPhiStatement(block, type);

	auto listNode = tree[1];
	for(auto phiEntryNodePtr : listNode) {
		ctx_.AddPhiPlaceholder(phistatement, phiEntryNodePtr->GetString());
	}

	return phistatement;
}


SSAStatement* StatementAssembler::parse_statement(const GenCSSA::AstNode &statement_body, SSABlock* block)
{
	switch(statement_body.GetType()) {
		case GenCSSANodeType::StatementBankRegRead:
			return parse_bank_reg_read_statement(statement_body, block);
		case GenCSSANodeType::StatementBankRegWrite:
			return parse_bank_reg_write_statement(statement_body, block);
		case GenCSSANodeType::StatementBinary:
			return parse_binary_statement(statement_body, block);
		case GenCSSANodeType::StatementCall:
			return parse_call_statement(statement_body, block);
		case GenCSSANodeType::StatementCast:
			return parse_cast_statement(statement_body, block);
		case GenCSSANodeType::StatementConstant:
			return parse_constant_statement(statement_body, block);
		case GenCSSANodeType::StatementDevRead:
			return parse_devread_statement(statement_body, block);
		case GenCSSANodeType::StatementDevWrite:
			return parse_devread_statement(statement_body, block);
		case GenCSSANodeType::StatementIf:
			return parse_if_statement(statement_body, block);
		case GenCSSANodeType::StatementIntrinsic:
			return parse_intrinsic_statement(statement_body, block);
		case GenCSSANodeType::StatementJump:
			return parse_jump_statement(statement_body, block);
		case GenCSSANodeType::StatementMemRead:
			return parse_mem_read_statement(statement_body, block);
		case GenCSSANodeType::StatementMemWrite:
			return parse_mem_write_statement(statement_body, block);
		case GenCSSANodeType::StatementVariableRead:
			return parse_read_statement(statement_body, block);
		case GenCSSANodeType::StatementRegRead:
			return parse_reg_read_statement(statement_body, block);
		case GenCSSANodeType::StatementRegWrite:
			return parse_reg_write_statement(statement_body, block);
		case GenCSSANodeType::StatementSelect:
			return parse_select_statement(statement_body, block);
		case GenCSSANodeType::StatementStructMember:
			return parse_struct_statement(statement_body, block);
		case GenCSSANodeType::StatementSwitch:
			return parse_switch_statement(statement_body, block);
		case GenCSSANodeType::StatementUnary:
			return parse_unop_statement(statement_body, block);
		case GenCSSANodeType::StatementVExtract:
			return parse_vextract_statement(statement_body, block);
		case GenCSSANodeType::StatementVInsert:
			return parse_vinsert_statement(statement_body, block);
		case GenCSSANodeType::StatementVariableWrite:
			return parse_write_statement(statement_body, block);
		case GenCSSANodeType::StatementRaise:
			return parse_raise_statement(statement_body, block);
		case  GenCSSANodeType::StatementReturn:
			return parse_return_statement(statement_body, block);
		case  GenCSSANodeType::StatementReturnVoid:
			return parse_return_statement(statement_body, block);
		case GenCSSANodeType::StatementPhi:
			return parse_phi_statement(statement_body, block);
		default:
			UNEXPECTED;
	}
}


SSAStatement* StatementAssembler::Assemble(const GenCSSA::AstNode &tree, SSABlock *block)
{
	GASSERT(tree.GetType() == GenCSSANodeType::Statement);

	auto stmt_name = tree[0].GetString();
	auto &statement_body = tree[1];

	SSAStatement *stmt = parse_statement(statement_body, block);
	// stmt->SetDiag(DiagNode("asm", tree));
	ctx_.Put(stmt_name, stmt);
	return stmt;
}

SSAStatement* StatementAssembler::get_statement(const GenCSSA::AstNode &stmt_id_node)
{
	auto stmt_name = stmt_id_node.GetString();
	auto uncast_stmt = ctx_.Get(stmt_name);
	auto stmt = dynamic_cast<SSAStatement*>(uncast_stmt);

	if(stmt == nullptr) {
		throw std::invalid_argument("Could not find statement");
	}

	return stmt;
}

SSASymbol* StatementAssembler::get_symbol(const GenCSSA::AstNode &sym_id_node)
{
	auto sym_name = sym_id_node.GetString();
	auto sym = dynamic_cast<SSASymbol*>(ctx_.Get(sym_name));

	if(sym == nullptr) {
		throw std::invalid_argument("Could not find symbol");
	}

	return sym;
}

SSABlock* StatementAssembler::get_block(const GenCSSA::AstNode &block_id_node)
{
	auto block_name = block_id_node.GetString();
	auto target = dynamic_cast<SSABlock*>(ctx_.Get(block_name));
	if(target == nullptr) {
		throw std::invalid_argument("Could not find block");
	}
	return target;
}
