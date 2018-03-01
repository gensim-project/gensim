/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "ssa_asm/ssa_asmLexer.h"
#include "ssa_asm/ssa_asmParser.h"

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
static IRConstant parse_constant_value(pANTLR3_BASE_TREE tree)
{
	auto text = (char*)tree->getText(tree)->chars;
	switch(tree->getType(tree)) {
		case SSAASM_INT:
			return IRConstant::Integer(strtoull(text, nullptr, 10));
		case SSAASM_FLOAT:
			return IRConstant::Float(strtof(text, nullptr));
		case SSAASM_DOUBLE:
			return IRConstant::Double(strtod(text, nullptr));
		default:
			UNREACHABLE;
	}


}

static SSAType parse_type(SSAContext &ctx, pANTLR3_BASE_TREE tree)
{
	GASSERT(tree->getType(tree) == SSAASM_TYPE);
	gensim::genc::IRType out;
	char *text = (char*)tree->getText(tree)->chars;

	if(strcmp(text, "Instruction") == 0) {
		return ctx.GetTypeManager().GetStructType(text);
	}

	if(!gensim::genc::IRType::ParseType(text, out)) {
		throw std::logic_error("Could not parse type");
	}

	for(unsigned i = 0; i < tree->getChildCount(tree); ++i) {
		auto childnode = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
		switch(childnode->getType(childnode)) {
			case SSAASM_TYPE_VECTOR:
				out.VectorWidth = parse_constant_value((pANTLR3_BASE_TREE)childnode->getChild(childnode, 0)).Int();
				break;
			case SSAASM_TYPE_REF:
				out.Reference = true;
				break;
			default:
				UNREACHABLE;
		}
	}

	return out;
}


void ContextAssembler::SetTarget(SSAContext* target)
{
	target_ = target;
}

bool ContextAssembler::Assemble(AssemblyFileContext &afc, DiagnosticContext &dc)
{
	pANTLR3_BASE_TREE tree = (pANTLR3_BASE_TREE)afc.GetTree();
	if(tree->getType(tree) != CONTEXT) {
		throw std::logic_error("");
	}

	try {
		ActionAssembler actionasm;
		ExternalActionAssembler xactionasm;
		for(unsigned i = 0; i < tree->getChildCount(tree); ++i) {
			auto action_node = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
			SSAActionBase *action = nullptr;
			if(action_node->getType(action_node) == ACTION) {
				action = actionasm.Assemble(action_node, *target_);
			} else if(action_node->getType(action_node) == ACTION_EXTERNAL) {
				action = xactionasm.Assemble(action_node, *target_);
			} else {
				throw std::logic_error("Unexpected node type");
			}
			target_->AddAction(action);
		}
	} catch (std::exception &e) {
		dc.Error(e.what());
		return false;
	} catch(gensim::Exception &e) {
		dc.Error(e.what());
		return false;
	}

	return true;
}

SSAExternalAction* ExternalActionAssembler::Assemble(pANTLR3_BASE_TREE tree, SSAContext& context)
{
	if(tree->getType(tree) != ACTION_EXTERNAL) {
		throw std::logic_error("");
	}
	AssemblyContext ctx;
	auto action_id_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	std::string action_id = (char*)action_id_node->getText(action_id_node)->chars;

	auto action_rtype_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	SSAType return_type = parse_type(context, action_rtype_node);

	auto attribute_list_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	IRSignature::param_type_list_t params;
	auto action_params_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 3);
	for(unsigned i = 0; i < action_params_node->getChildCount(action_params_node); i += 2) {
		auto type_node = (pANTLR3_BASE_TREE)action_params_node->getChild(action_params_node,i);
		std::string name = "p" + std::to_string(i);

		IRParam param (name, parse_type(context, type_node));
		params.push_back(param);
	}

	IRSignature signature (action_id, return_type, params);
	signature.AddAttribute(gensim::genc::ActionAttribute::External);

	for(unsigned i = 0; i < attribute_list_node->getChildCount(attribute_list_node); ++i) {
		auto attribute_node = (pANTLR3_BASE_TREE)attribute_list_node->getChild(attribute_list_node, i);
		switch(attribute_node->getType(attribute_node)) {
			case ATTRIBUTE_NOINLINE:
				signature.AddAttribute(ActionAttribute::NoInline);
				break;
			case ATTRIBUTE_HELPER:
				signature.AddAttribute(ActionAttribute::Helper);
				break;
			default:
				UNREACHABLE;
		}
	}

	SSAActionPrototype prototype (signature);
	return new SSAExternalAction(context, prototype);
}


SSAFormAction* ActionAssembler::Assemble(pANTLR3_BASE_TREE tree, SSAContext &context)
{
	if(tree->getType(tree) != ACTION) {
		throw std::logic_error("");
	}
	AssemblyContext ctx;
	auto action_id_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	std::string action_id = (char*)action_id_node->getText(action_id_node)->chars;

	auto action_rtype_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	SSAType return_type = parse_type(context, action_rtype_node);

	auto attribute_list_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	IRSignature::param_type_list_t params;
	auto action_params_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 3);
	for(unsigned i = 0; i < action_params_node->getChildCount(action_params_node); i += 2) {
		auto type_node = (pANTLR3_BASE_TREE)action_params_node->getChild(action_params_node,i);
		auto name_node = (pANTLR3_BASE_TREE)action_params_node->getChild(action_params_node,i+1);
		auto name_string = (char*)name_node->getText(name_node)->chars;

		IRParam param (name_string, parse_type(context, type_node));
		params.push_back(param);
	}

	IRSignature signature (action_id, return_type, params);

	for(unsigned i = 0; i < attribute_list_node->getChildCount(attribute_list_node); ++i) {
		auto attribute_node = (pANTLR3_BASE_TREE)attribute_list_node->getChild(attribute_list_node, i);
		switch(attribute_node->getType(attribute_node)) {
			case ATTRIBUTE_NOINLINE:
				signature.AddAttribute(ActionAttribute::NoInline);
				break;
			case ATTRIBUTE_HELPER:
				signature.AddAttribute(ActionAttribute::Helper);
				break;
			default:
				UNREACHABLE;
		}
	}

	SSAActionPrototype prototype (signature);
	SSAFormAction *action = new SSAFormAction(context, prototype);
	action->Arch = &context.GetArchDescription();
	action->Isa = &context.GetIsaDescription();

	// map parameter names to params
	for(unsigned i = 0; i < action_params_node->getChildCount(action_params_node); i += 2) {
		auto name_node = (pANTLR3_BASE_TREE)action_params_node->getChild(action_params_node,i+1);
		auto name_string = (char*)name_node->getText(name_node)->chars;

		action->ParamSymbols.at(i/2)->SetName(name_string);
		ctx.Put(name_string, action->ParamSymbols.at(i/2));
	}

	auto action_syms_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 4);
	// 2 nodes per symbol: type, name
	for(unsigned i = 0; i < action_syms_node->getChildCount(action_syms_node); i += 2) {
		auto type_node = (pANTLR3_BASE_TREE)action_syms_node->getChild(action_syms_node,i);
		auto name_node = (pANTLR3_BASE_TREE)action_syms_node->getChild(action_syms_node,i+1);
		auto name_string = (char*)name_node->getText(name_node)->chars;

		SSASymbol *sym = new SSASymbol(context, name_string, parse_type(context, type_node), Symbol_Local);
		sym->SetName(name_string);
		action->AddSymbol(sym);
		ctx.Put(name_string, sym);
	}

	// Parse blocks
	auto block_def_list_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 5);
	for(unsigned i = 0; i < block_def_list_node->getChildCount(block_def_list_node); ++i) {
		auto block_name_node = (pANTLR3_BASE_TREE)block_def_list_node->getChild(block_def_list_node, i);
		SSABlock *block = new SSABlock(action->GetContext(), *action);
		ctx.Put((char*)block_name_node->getText(block_name_node)->chars, block);

		if(i == 0) {
			action->EntryBlock = block;
		}
	}

	auto block_list_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 6);
	BlockAssembler ba (ctx);
	for(unsigned i = 0; i < block_list_node->getChildCount(block_list_node); ++i) {
		ba.Assemble((pANTLR3_BASE_TREE)block_list_node->getChild(block_list_node, i), *action);
	}

	ctx.ProcessPhiStatements();

	return action;
}

BlockAssembler::BlockAssembler(AssemblyContext& ctx) : ctx_(ctx)
{

}


SSABlock* BlockAssembler::Assemble(pANTLR3_BASE_TREE tree, SSAFormAction &action)
{
	if(tree->getType(tree) != BLOCK) {
		throw std::logic_error("");
	}

	// parse block name
	auto block_name_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	std::string block_name = (char*)block_name_node->getText(block_name_node)->chars;

	SSABlock *block = dynamic_cast<SSABlock*>(ctx_.Get(block_name));
	if(block == nullptr) {
		throw std::logic_error("");
	}

	auto block_statement_list = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	if(block_statement_list->getType(block_statement_list) != BLOCK_STATEMENTS) {
		throw std::logic_error("");
	}
	StatementAssembler sa (action, ctx_);
	for(unsigned i = 0; i < block_statement_list->getChildCount(block_statement_list); ++i) {
		sa.Assemble((pANTLR3_BASE_TREE)block_statement_list->getChild(block_statement_list, i), block);
	}

	return block;
}

static gensim::genc::BinaryOperator::EBinaryOperator parse_binop(pANTLR3_BASE_TREE tree)
{
	using namespace gensim::genc::BinaryOperator;
	switch(tree->getType(tree)) {
		case BINOP_PLUS :
			return Add;
		case NEGATIVE :
			return Subtract;
		case BINOP_MUL :
			return Multiply;
		case BINOP_DIV :
			return Divide;
		case BINOP_MOD :
			return Modulo;
		case BINOP_SAR:
			return SignedShiftRight;
		case BINOP_SHR:
			return ShiftRight;
		case BINOP_SHL:
			return ShiftLeft;
		case BINOP_ROR:
			return RotateRight;
		case BINOP_ROL:
			return RotateLeft;
		case AMPERSAND:
			return Bitwise_And;
		case BINOP_OR:
			return Bitwise_Or;
		case BINOP_XOR:
			return Bitwise_XOR;
		case CMP_EQUALS:
			return Equality;
		case CMP_NOTEQUALS:
			return Inequality;
		case CMP_GT:
			return GreaterThan;
		case CMP_GTE:
			return GreaterThanEqual;
		case CMP_LT:
			return LessThan;
		case CMP_LTE:
			return LessThanEqual;
		default:
			throw std::logic_error("Unexpected binary operator " + std::string((char*)tree->getText(tree)->chars));
	}
}

StatementAssembler::StatementAssembler(SSAFormAction& action, AssemblyContext &ctx) : action_(action), ctx_(ctx)
{

}

SSAStatement *StatementAssembler::parse_bank_reg_read_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_BANKREGREAD);

	auto bank_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto index_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	IRConstant bank = parse_constant_value(bank_node);
	SSAStatement *idx = get_statement(index_node);

	return SSARegisterStatement::CreateBankedRead(block, bank.Int(), idx);
}

SSAStatement *StatementAssembler::parse_bank_reg_write_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_BANKREGWRITE);

	auto bank_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto index_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	IRConstant bank = parse_constant_value(bank_node);
	SSAStatement *idx = get_statement(index_node);
	SSAStatement *value = get_statement(value_node);

	return SSARegisterStatement::CreateBankedWrite(block, bank.Int(), idx, value);
}

SSAStatement *StatementAssembler::parse_binary_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_BINOP);

	auto op_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto lhs_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto rhs_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	auto lhs_statement = get_statement(lhs_node);
	auto rhs_statement = get_statement(rhs_node);

	auto statement_type = parse_binop(op_node);
	if(statement_type == BinaryOperator::ShiftRight && lhs_statement->GetType().Signed) {
		statement_type = BinaryOperator::SignedShiftRight;
	}

	return new SSABinaryArithmeticStatement(block, lhs_statement, rhs_statement, statement_type);
}

SSAStatement *StatementAssembler::parse_call_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_CALL);

	auto target_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	std::string target_name = (char*)target_node->getText(target_node)->chars;
	SSAActionBase *target = action_.GetContext().GetAction(target_name);

	std::vector<SSAValue*> params;
	for(unsigned i = 1; i < tree->getChildCount(tree); ++i) {
		int param_index = i-1;
		auto &param = target->GetPrototype().ParameterTypes().at(param_index);
		if(param.Reference) {
			params.push_back(get_symbol((pANTLR3_BASE_TREE)tree->getChild(tree, i)));
		} else {
			params.push_back(get_statement((pANTLR3_BASE_TREE)tree->getChild(tree, i)));
		}
	}

	return new SSACallStatement(block, (SSAFormAction*)target, params);
}

SSACastStatement::CastType parse_cast_type(pANTLR3_BASE_TREE node)
{
	GASSERT(node->getType(node) == SSAASM_ID);
	std::string node_text = (char*)node->getText(node)->chars;

	if(node_text == "zextend") {
		return SSACastStatement::Cast_ZeroExtend;
	} else if(node_text == "sextend") {
		return SSACastStatement::Cast_SignExtend;
	} else if(node_text == "truncate") {
		return SSACastStatement::Cast_Truncate;
	} else if(node_text == "convert") {
		return SSACastStatement::Cast_Convert;
	} else if(node_text == "reinterpret") {
		return SSACastStatement::Cast_Reinterpret;
	} else {
		UNEXPECTED;
	}
}

SSACastStatement::CastOption parse_cast_option(pANTLR3_BASE_TREE node)
{
	GASSERT(node->getType(node) == SSAASM_ID);
	std::string node_text = (char*)node->getText(node)->chars;

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

SSAStatement *StatementAssembler::parse_cast_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_CAST);

	auto type_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto expr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto cast_type_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	SSACastStatement::CastType ctype = parse_cast_type(cast_type_node);

	SSACastStatement::CastOption option = SSACastStatement::Option_None;
	if(tree->getChildCount(tree) == 4) {
		auto option_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 3);
		option = parse_cast_option(option_node);
	}

	auto target_type = parse_type(action_.GetContext(), type_node);
	auto statement = get_statement(expr_node);

	auto cast_statement = new SSACastStatement(block, target_type, statement, ctype);
	cast_statement->SetOption(option);
	return cast_statement;
}

SSAStatement *StatementAssembler::parse_constant_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_CONSTANT);

	auto type_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	IRConstant constant_value = parse_constant_value(value_node);
	SSAType constant_type = parse_type(action_.GetContext(), type_node);

	if(SSAType::Cast(constant_value, constant_type, constant_type) != constant_value) {
		throw std::logic_error("Constant type cannot contain specified value");
	}

	return new SSAConstantStatement(block, constant_value, constant_type);
}

SSAStatement *StatementAssembler::parse_devread_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_DEVREAD);

	auto dev_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto addr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto target_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	return new SSADeviceReadStatement(block, get_statement(dev_node), get_statement(addr_node), get_symbol(target_node));
}

SSAStatement *StatementAssembler::parse_devwrite_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	UNIMPLEMENTED;
	/*
	GASSERT(tree->getType(tree) == STATEMENT_DEVWRITE);

	auto dev_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto addr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	return new SSADeviceWriteStatement(block, get_statement(dev_node), get_statement(addr_node), get_statement(value_node));
	 */
}

SSAStatement *StatementAssembler::parse_if_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_IF);

	auto expr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto true_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto false_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	return new SSAIfStatement(block, get_statement(expr_node), *get_block(true_node), *get_block(false_node));
}

SSAStatement *StatementAssembler::parse_intrinsic_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_INTRINSIC);

	auto id_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	uint32_t id = parse_constant_value(id_node).Int();

	SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(block, (gensim::genc::ssa::SSAIntrinsicStatement::IntrinsicType)id);

	for(unsigned i = 1; i < tree->getChildCount(tree); ++i) {
		auto stmt_node = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
		intrinsic->AddArg(get_statement(stmt_node));
	}

	return intrinsic;
}

SSAStatement *StatementAssembler::parse_jump_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_JUMP);

	auto target_name_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);

	SSABlock *target = get_block(target_name_node);

	return new SSAJumpStatement(block, *target);
}

SSAStatement *StatementAssembler::parse_mem_read_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_MEMREAD);

	auto width_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto addr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto target_name_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	IRConstant width = parse_constant_value(width_node);
	SSAStatement *addr = get_statement(addr_node);
	SSASymbol *target = get_symbol(target_name_node);

	return &SSAMemoryReadStatement::CreateRead(block, addr, target, width.Int(), false);
}

SSAStatement *StatementAssembler::parse_mem_write_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_MEMWRITE);

	auto width_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto addr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	IRConstant width = parse_constant_value(width_node);
	SSAStatement *addr = get_statement(addr_node);
	SSAStatement *target = get_statement(value_node);

	return &SSAMemoryWriteStatement::CreateWrite(block, addr, target, width.Int());
}

SSAStatement *StatementAssembler::parse_read_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_READ);

	auto target_name_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	return new SSAVariableReadStatement(block, get_symbol(target_name_node));
}


SSAStatement *StatementAssembler::parse_reg_read_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_REGREAD);

	auto target_reg_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto target_regid = parse_constant_value(target_reg_node);

	return SSARegisterStatement::CreateRead(block, target_regid.Int());
}

SSAStatement *StatementAssembler::parse_reg_write_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_REGWRITE);

	auto target_reg_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto target_regid = parse_constant_value(target_reg_node);
	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	return SSARegisterStatement::CreateWrite(block, target_regid.Int(), get_statement(value_node));
}

SSAStatement *StatementAssembler::parse_raise_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_RAISE);
	return new SSARaiseStatement(block, nullptr);
}

SSAStatement *StatementAssembler::parse_return_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_RETURN);

	if(tree->getChildCount(tree) == 0) {
		return new SSAReturnStatement(block, nullptr);
	} else {
		auto target_name_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
		return new SSAReturnStatement(block, get_statement(target_name_node));
	}
}

SSAStatement *StatementAssembler::parse_select_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_SELECT);

	auto expr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto true_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto false_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	return new SSASelectStatement(block, get_statement(expr_node), get_statement(true_node), get_statement(false_node));
}

SSAStatement *StatementAssembler::parse_struct_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_STRUCT);
	auto struct_var_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto member_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto member_name = (char*)member_node->getText(member_node)->chars;

	auto struct_symbol = get_symbol(struct_var_node);
	GASSERT(struct_symbol->GetType().IsStruct());
	GASSERT(struct_symbol->GetType().BaseType.StructType->HasMember(member_name));

	return new SSAReadStructMemberStatement(block, get_symbol(struct_var_node), member_name);
}

SSAStatement *StatementAssembler::parse_switch_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_SWITCH);
	auto expr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto default_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	SSASwitchStatement *stmt = new SSASwitchStatement(block, get_statement(expr_node), get_block(default_node));
	for(unsigned i = 2; i < tree->getChildCount(tree); i += 2) {
		auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
		auto target_node = (pANTLR3_BASE_TREE)tree->getChild(tree, i+1);

		stmt->AddValue(get_statement(value_node), get_block(target_node));
	}

	return stmt;
}

SSAStatement *StatementAssembler::parse_unop_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_UNOP);

	auto op_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto expr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	SSAUnaryOperator::ESSAUnaryOperator op_type;

	switch(op_node->getType(op_node)) {
		case EXCLAMATION:
			op_type = SSAUnaryOperator::OP_NEGATE;
			break;
		case COMPLEMENT:
			op_type = SSAUnaryOperator::OP_COMPLEMENT;
			break;
		case NEGATIVE:
			op_type = SSAUnaryOperator::OP_NEGATIVE;
			break;
		default:
			UNREACHABLE;
	}

	return new SSAUnaryArithmeticStatement(block, get_statement(expr_node), op_type);
}

SSAStatement *StatementAssembler::parse_vextract_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_VEXTRACT);

	auto base_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto index_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	return new SSAVectorExtractElementStatement(block, get_statement(base_node), get_statement(index_node));
}

SSAStatement *StatementAssembler::parse_vinsert_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_VINSERT);

	auto base_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto index_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 2);

	return new SSAVectorInsertElementStatement(block, get_statement(base_node), get_statement(index_node), get_statement(value_node));
}

SSAStatement *StatementAssembler::parse_write_statement(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT_WRITE);

	auto target_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto target = get_symbol(target_node);

	auto value_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
	auto value = get_statement(value_node);

	return new SSAVariableWriteStatement(block, target, value);
}

SSAStatement* StatementAssembler::parse_phi_statement(pANTLR3_BASE_TREE tree, SSABlock* block)
{
	GASSERT(tree->getType(tree) == STATEMENT_PHI);

	auto type_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto type = parse_type(block->GetContext(), type_node);

	auto phistatement = new SSAPhiStatement(block, type);

	for(unsigned i = 1; i < tree->getChildCount(tree); ++i) {
		auto childnode = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
		ctx_.AddPhiPlaceholder(phistatement, (char*)childnode->getText(childnode)->chars);
	}

	return phistatement;
}


SSAStatement* StatementAssembler::parse_statement(pANTLR3_BASE_TREE statement_body, SSABlock* block)
{
	switch(statement_body->getType(statement_body)) {
		case STATEMENT_BANKREGREAD:
			return parse_bank_reg_read_statement(statement_body, block);
		case STATEMENT_BANKREGWRITE:
			return parse_bank_reg_write_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_BINOP:
			return parse_binary_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_CALL:
			return parse_call_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_CAST:
			return parse_cast_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_CONSTANT:
			return parse_constant_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_DEVREAD:
			return parse_devread_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_DEVWRITE:
			return parse_devread_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_IF:
			return parse_if_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_INTRINSIC:
			return parse_intrinsic_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_JUMP:
			return parse_jump_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_MEMREAD:
			return parse_mem_read_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_MEMWRITE:
			return parse_mem_write_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_READ:
			return parse_read_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_REGREAD:
			return parse_reg_read_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_REGWRITE:
			return parse_reg_write_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_SELECT:
			return parse_select_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_STRUCT:
			return parse_struct_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_SWITCH:
			return parse_switch_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_UNOP:
			return parse_unop_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_VEXTRACT:
			return parse_vextract_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_VINSERT:
			return parse_vinsert_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_WRITE:
			return parse_write_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_RAISE:
			return parse_raise_statement((pANTLR3_BASE_TREE)statement_body, block);
		case  STATEMENT_RETURN:
			return parse_return_statement((pANTLR3_BASE_TREE)statement_body, block);
		case STATEMENT_PHI:
			return parse_phi_statement((pANTLR3_BASE_TREE)statement_body, block);
		default:
			throw std::logic_error("Unknown assembly statement type " + std::string((char*)statement_body->getText(statement_body)->chars));
	}
}


SSAStatement* StatementAssembler::Assemble(pANTLR3_BASE_TREE tree, SSABlock *block)
{
	GASSERT(tree->getType(tree) == STATEMENT);

	auto stmt_name_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	auto stmt_name = (char*)stmt_name_node->getText(stmt_name_node)->chars;
	auto statement_body = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	SSAStatement *stmt = parse_statement(statement_body, block);
	stmt->SetDiag(DiagNode("asm", tree));
	ctx_.Put(stmt_name, stmt);
	return stmt;
}

SSAStatement* StatementAssembler::get_statement(pANTLR3_BASE_TREE stmt_id_node)
{
	auto stmt_name = (char*)stmt_id_node->getText(stmt_id_node)->chars;
	auto stmt = dynamic_cast<SSAStatement*>(ctx_.Get(stmt_name));

	if(stmt == nullptr) {
		throw std::invalid_argument("Could not find statement");
	}

	return stmt;
}

SSASymbol* StatementAssembler::get_symbol(pANTLR3_BASE_TREE sym_id_node)
{
	auto sym_name = (char*)sym_id_node->getText(sym_id_node)->chars;
	auto sym = dynamic_cast<SSASymbol*>(ctx_.Get(sym_name));

	if(sym == nullptr) {
		throw std::invalid_argument("Could not find symbol");
	}

	return sym;
}

SSABlock* StatementAssembler::get_block(pANTLR3_BASE_TREE block_id_node)
{
	auto block_name = (char*)block_id_node->getText(block_id_node)->chars;
	auto target = dynamic_cast<SSABlock*>(ctx_.Get(block_name));
	if(target == nullptr) {
		throw std::invalid_argument("Could not find block");
	}
	return target;
}
