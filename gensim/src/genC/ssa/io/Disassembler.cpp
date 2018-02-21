/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"

#include "genC/ssa/io/Disassemblers.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/analysis/CallGraphAnalysis.h"

using namespace gensim::genc::ssa::io;
using namespace gensim::genc::ssa;

const std::string indent = "  ";

void ContextDisassembler::Disassemble(const SSAContext* context, std::ostream& str)
{
	ActionDisassembler ad;

	CallGraphAnalysis cga;
	auto cg = cga.Analyse(*context);

	// try and disassemble the actions in order, such that all dependencies
	// of each action are disassembled before that action

	std::vector<SSAActionBase*> action_list;
	std::set<SSAActionBase *> action_set;
	while(action_list.size() < context->Actions().size()) {
		for(auto action : context->Actions()) {
			if(action_set.count(action.second)) {
				continue;
			}

			auto callees = cg.GetCallees((SSAFormAction*)action.second);
			
			std::set<SSAActionBase*> intersection;
			std::set_intersection(action_set.begin(), action_set.end(), callees.begin(), callees.end(), std::inserter(intersection, intersection.begin()));
			if(intersection == callees) {
				action_list.push_back(action.second);
				action_set.insert(action.second);
			}
		}
	}

	assert(action_list.size() == context->Actions().size());
	
	for(auto action : action_list) {
		ad.Disassemble(action, str);
	}
}
void ActionDisassembler::Disassemble(SSAActionBase* baseaction, std::ostream& str)
{
	const SSAFormAction *action = dynamic_cast<const SSAFormAction*>(baseaction);
	if(action == nullptr) {
		throw std::logic_error("");
	}

	TypeDisassembler td;

	str << "action ";
	td.Disassemble(action->GetPrototype().ReturnType(), str);
	str << " " << action->GetPrototype().GetIRSignature().GetName();

	// disassemble attributes
	for(auto i : action->GetPrototype().GetIRSignature().GetAttributes()) {
		switch(i) {
			case ActionAttribute::NoInline:
				str << " noinline";
				break;
			case ActionAttribute::Helper:
				str << " helper";
				break;
			default:
				UNREACHABLE;
		}
	}

	str <<" (";
	// parameters
	for(SSASymbol *param : action->ParamSymbols) {

		str << std::endl << indent << indent;
		td.Disassemble(param->GetType(), str);
		str << " " << param->GetName();
	}

	str << std::endl << indent << ") [";
	// symbols
	// build an ordered list of symbols
	std::map<std::string, SSASymbol*> ordered_syms;

	for(SSASymbol *sym : action->Symbols()) {
		if(sym->SType == Symbol_Parameter) {
			continue;
		}
		ordered_syms[sym->GetName()] = sym;
	}

	for(auto pr : ordered_syms) {
		SSASymbol *sym = pr.second;
		str << std::endl << indent << indent;
		td.Disassemble(sym->GetType(), str);
		str << " " << pr.first;
	}

	str << std::endl << indent << "] ";

	str << "<" << std::endl;
	str << indent << indent << action->EntryBlock->GetName() << std::endl;
	for(auto block : action->Blocks) {
		if(block == action->EntryBlock) {
			continue;
		}
		str << indent<< indent << block->GetName() << std::endl;
	}
	str<< indent << ">";

	str << " {" << std::endl;
	// blocks
	BlockDisassembler bd;
	for(auto block : action->Blocks) {
		bd.Disassemble(block, str);
	}
	str << "}" << std::endl;
}


void BlockDisassembler::Disassemble(SSABlock* block, std::ostream& str)
{
	StatementDisassembler sd;

	str << indent << "block " << block->GetName() << " {" << std::endl;
	for(auto *stmt : block->GetStatements()) {
		str << indent << indent;
		sd.Disassemble(stmt, str);
		str << std::endl;
	}
	str << indent << "}" << std::endl;
}

void TypeDisassembler::Disassemble(const SSAType& type, std::ostream& str)
{
	switch(type.DataType) {
		case SSAType::PlainOldData:
			DisassemblePOD(type, str);
			break;
		case SSAType::Struct:
			DisassembleStruct(type, str);
			break;
		default:
			throw std::logic_error("");
	}
	if(type.VectorWidth != 1) {
		str << "[" << (uint32_t)type.VectorWidth << "]";
	}
	if(type.Reference) {
		str << "&";
	}
}

void TypeDisassembler::DisassemblePOD(const SSAType& type, std::ostream& str)
{
	GASSERT(type.DataType == SSAType::PlainOldData);

	if(type.IsFloating()) {
		switch(type.BaseType.PlainOldDataType) {
			case IRPlainOldDataType::FLOAT:
				str << "float";
				break;
			case IRPlainOldDataType::DOUBLE:
				str << "double";
				break;
			default:
				throw std::logic_error("");
		}
	} else {
		if(type.BaseType.PlainOldDataType == IRPlainOldDataType::VOID) {
			str << "void";
			return;
		}

		if(type.Signed) {
			str << "s";
		} else {
			str << "u";
		}
		switch(type.BaseType.PlainOldDataType) {
			case IRPlainOldDataType::INT8:
				str << "int8";
				break;
			case IRPlainOldDataType::INT16:
				str << "int16";
				break;
			case IRPlainOldDataType::INT32:
				str << "int32";
				break;
			case IRPlainOldDataType::INT64:
				str << "int64";
				break;
			default:
				throw std::logic_error("");
		}
	}
}

void TypeDisassembler::DisassembleStruct(const SSAType& type, std::ostream& str)
{
	GASSERT(type.DataType == SSAType::Struct);
	str << type.BaseType.StructType->Name;
}


class StatementDisassemblerVisitor : public SSAStatementVisitor
{
public:
	StatementDisassemblerVisitor(std::ostream &str) : str_(str) {}

	std::string Header(SSAStatement &stmt)
	{
		std::stringstream str;
		str << stmt.GetName();
		return str.str();
	}

	std::string DisassembleType(const SSAType &type)
	{
		std::stringstream str;
		TypeDisassembler td;
		td.Disassemble(type, str);
		return str.str();
	}

	void VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt) override
	{
		str_ << Header(stmt) << " = binary " << gensim::genc::BinaryOperator::PrettyPrintOperator(stmt.Type) << " "<< stmt.LHS()->GetName() << " " << stmt.RHS()->GetName() << ";";
	}
	void VisitBitDepositStatement(SSABitDepositStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitBitExtractStatement(SSABitExtractStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitCallStatement(SSACallStatement& stmt) override
	{
		if(stmt.Target()->GetPrototype().ReturnType() != gensim::genc::IRTypes::Void) {
			str_ << Header(stmt) << " = ";
		} else {
			str_ << Header(stmt) << ": ";
		}
		str_ << "call " << stmt.Target()->GetPrototype().GetIRSignature().GetName();
		for(unsigned i = 0; i < stmt.ArgCount(); ++i) {
			str_ << " " << stmt.Arg(i)->GetName();
		}
		str_ << ";";
	}
	void VisitCastStatement(SSACastStatement& stmt) override
	{
		std::string cast_type;
		std::string cast_option;
		switch(stmt.GetCastType()) {
			case SSACastStatement::Cast_ZeroExtend: cast_type = "zextend"; break;
			case SSACastStatement::Cast_SignExtend: cast_type = "sextend"; break;
			case SSACastStatement::Cast_Truncate: cast_type = "truncate"; break;
			case SSACastStatement::Cast_Convert: cast_type = "convert"; break;
			case SSACastStatement::Cast_Reinterpret: cast_type = "reinterpret"; break;
			default:
				UNEXPECTED;
		}
		switch(stmt.GetOption()) {
			case SSACastStatement::Option_None: cast_option = ""; break;
			case SSACastStatement::Option_RoundDefault: cast_option = "round_default"; break;
			case SSACastStatement::Option_RoundTowardNInfinity: cast_option = "round_ninfinity"; break;
			case SSACastStatement::Option_RoundTowardPInfinity: cast_option = "round_pinfinity"; break;
			case SSACastStatement::Option_RoundTowardNearest: cast_option = "round_nearest"; break;
			case SSACastStatement::Option_RoundTowardZero: cast_option = "round_zero"; break;
			default:
				UNEXPECTED;
		}
		
		
		str_ << Header(stmt) << " = cast " << cast_type << " " << cast_option << " " << DisassembleType(stmt.GetType()) << " " << stmt.Expr()->GetName() << ";";
	}
	void VisitConstantStatement(SSAConstantStatement& stmt) override
	{
		std::string constant_string;
		switch(stmt.Constant.Type()) {
				using gensim::genc::IRConstant;
			case IRConstant::Type_Integer:
				constant_string = std::to_string(stmt.Constant.Int());
				break;
			case IRConstant::Type_Float_Single:
				constant_string = std::to_string(stmt.Constant.Flt());
				break;
			case IRConstant::Type_Float_Double:
				constant_string = std::to_string(stmt.Constant.Dbl());
				break;
			default:
				throw std::logic_error("Unknown constant type");
				break;
		}
		str_ << Header(stmt) << " = constant " << DisassembleType(stmt.GetType()) << " " << constant_string << ";";
	}
	void VisitControlFlowStatement(SSAControlFlowStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitDeviceReadStatement(SSADeviceReadStatement& stmt) override
	{
		str_ << Header(stmt) << " = devread " << stmt.Device()->GetName() << " " << stmt.Address()->GetName() << " " << stmt.Target()->GetName() << ";";
	}
	void VisitIfStatement(SSAIfStatement& stmt) override
	{
		str_ << Header(stmt) << ": if " << stmt.Expr()->GetName() << " " << stmt.TrueTarget()->GetName() << " " << stmt.FalseTarget()->GetName() << ";";
	}
	void VisitIntrinsicStatement(SSAIntrinsicStatement& stmt) override
	{
		if(stmt.HasValue()) {
			str_ << Header(stmt) << " = intrinsic " << stmt.Type;
		} else {
			str_ << Header(stmt) << ": intrinsic " << stmt.Type;
		}
		for(unsigned i = 0; i < stmt.ArgCount(); ++i) {
			str_ << " " << stmt.Args(i)->GetName();
		}
		str_ << ";";
	}
	void VisitJumpStatement(SSAJumpStatement& stmt) override
	{
		str_ << Header(stmt) << ": jump " << stmt.Target()->GetName() << ";";
	}
	void VisitMemoryReadStatement(SSAMemoryReadStatement& stmt) override
	{
		str_ << Header(stmt) << ": memread " << (uint32_t)stmt.Width << " " << stmt.Addr()->GetName() << " " << stmt.Target()->GetName() << ";";
	}
	void VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt) override
	{
		str_ << Header(stmt) << ": memwrite " << (uint32_t)stmt.Width << " " << stmt.Addr()->GetName() << " " << stmt.Value()->GetName() << ";";
	}
	void VisitPhiStatement(SSAPhiStatement& stmt) override
	{
		str_ << Header(stmt) << " = phi " << DisassembleType(stmt.GetType()) << " [";

		for(auto i : stmt.Get()) {
			str_ << i->GetName() << " ";
		}

		str_ << "];";
	}
	void VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt) override
	{
		str_ << Header(stmt) << " = struct " << stmt.Target()->GetName() << " " << stmt.MemberName << ";";
	}
	void VisitRegisterStatement(SSARegisterStatement& stmt) override
	{
		if(stmt.IsRead) {
			if(stmt.IsBanked) {
				str_ << Header(stmt) << " = bankregread " << (uint32_t)stmt.Bank << " " << stmt.RegNum()->GetName() << ";";
			} else {
				str_ << Header(stmt) << " = regread " << (uint32_t)stmt.Bank << ";";
			}
		} else {
			if(stmt.IsBanked) {
				str_ << Header(stmt) << ": bankregwrite " << (uint32_t)stmt.Bank << " " << stmt.RegNum()->GetName() << " " << stmt.Value()->GetName() << ";";
			} else {
				str_ << Header(stmt) << ": regwrite " << (uint32_t)stmt.Bank << " " << stmt.Value()->GetName() << ";";
			}
		}
	}
	void VisitRaiseStatement(SSARaiseStatement& stmt) override
	{
		str_ << Header(stmt) << ": raise;";
	}
	void VisitReturnStatement(SSAReturnStatement& stmt) override
	{
		if(stmt.Value() != nullptr) {
			str_ << Header(stmt) << ": return " << stmt.Value()->GetName() << ";";
		} else {
			str_ << Header(stmt) << ": return;";
		}
	}
	void VisitSelectStatement(SSASelectStatement& stmt) override
	{
		str_ << Header(stmt) << " = select " << stmt.Cond()->GetName() << " " << stmt.TrueVal()->GetName() << " " << stmt.FalseVal()->GetName() << ";";
	}
	void VisitStatement(SSAStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitSwitchStatement(SSASwitchStatement& stmt) override
	{
		str_ << Header(stmt) << ": switch " << stmt.Expr()->GetName() << " " << stmt.Default()->GetName() << " [";
		// create an ordered map of value pairs
		std::map<std::string, std::pair<SSAStatement*, SSABlock*>> pairs;
		for(auto val : stmt.GetValues()) {
			pairs[val.first->GetName()] = val;
		}
		for(auto val : pairs) {
			str_ << val.second.first->GetName() << " " << val.second.second->GetName() << " ";
		}
		str_ << "];";
	}
	void VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt) override
	{
		str_ << Header(stmt) << " = unary " << gensim::genc::SSAUnaryOperator::PrettyPrint(stmt.Type) << " " << stmt.Expr()->GetName() << ";";
	}
	void VisitVariableKillStatement(SSAVariableKillStatement& stmt) override
	{
		UNIMPLEMENTED;
	}
	void VisitVariableReadStatement(SSAVariableReadStatement& stmt) override
	{
		str_ << Header(stmt) << " = read " << stmt.Target()->GetName() << ";";
	}
	void VisitVariableWriteStatement(SSAVariableWriteStatement& stmt) override
	{
		str_ << Header(stmt) << ": write " << stmt.Target()->GetName() << " " << stmt.Expr()->GetName() << ";";
	}
	void VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt) override
	{
		str_ << Header(stmt) << " = vextract " << stmt.Base()->GetName() << "[" << stmt.Index()->GetName() << "];";
	}
	void VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt) override
	{
		str_ << Header(stmt) << " = vinsert " << stmt.Base()->GetName() << "[" << stmt.Index()->GetName() << "] " << stmt.Value()->GetName() << ";";
	}

	virtual ~StatementDisassemblerVisitor()
	{

	}
private:
	std::ostream &str_;
};

void StatementDisassembler::Disassemble(SSAStatement* stmt, std::ostream& str)
{
	StatementDisassemblerVisitor sdv (str);
	stmt->Accept(sdv);
}

