/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSAReadStructMemberStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ir/IR.h"
#include "genC/Parser.h"

#include "isa/InstructionDescription.h"
#include "isa/ISADescription.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSAReadStructMemberStatement::SSAReadStructMemberStatement(SSABlock *parent, SSASymbol *target, const std::vector<std::string>& member, SSAStatement *before)
	: SSAStatement(Class_Unknown, 1, parent, before), MemberNames(member)
{
	SetTarget(target);
}

bool SSAReadStructMemberStatement::IsFixed() const
{
	return true;
}

bool SSAReadStructMemberStatement::HasSideEffects() const
{
	return false;
}


bool SSAReadStructMemberStatement::Resolve(DiagnosticContext &ctx)
{
	SSAFormAction *my_action = Parent->Parent;
	const IRType &struct_type = this->Target()->GetType();

	if(!struct_type.IsStruct()) {
		ctx.Error("Cannot access a member of a non struct type", GetDiag());
		return false;
	}

	bool found_member = true;
	// look for correct member in struct
	auto curr_type = Target()->GetType();
	for(auto member : MemberNames) {
		if(curr_type.IsStruct() && curr_type.BaseType.StructType->HasMember(member)) {
			curr_type = curr_type.BaseType.StructType->GetMemberType(member);
		} else {
			found_member = false;
			break;
		}
	}

	if(!found_member) {
		ctx.Error("Struct does not have field", GetDiag());
		return false;
	}

	return true;

//	isa::InstructionDescription *insn = ctx.GetRegisteredInstructionFor(Parent->Parent->GetAction());
//
//	if(!insn || insn->Format->hasChunk(MemberName) || (insn->ISA.Get_Disasm_Fields().find(MemberName) != insn->ISA.Get_Disasm_Fields().end()) || MemberName == "IsPredicated" || MemberName == "Instr_Length")
//		return SSAStatement::Resolve(ctx);
//	else {
//		ctx.Diag().Error("Instruction does not have field " + MemberName + " in instruction " + insn->Name, ctx.CurrFilename, GetStatement()->GetNode());
//		return false;
//	}
}

std::string SSAReadStructMemberStatement::FormatMembers() const
{
	std::string output;
	for(auto i : MemberNames) {
		output += "." + i;
	}
	return output;
}


SSAType SSAReadStructMemberStatement::ResolveType(const SSASymbol *target, const std::vector<std::string>& member_name)
{
	GASSERT(target->GetType().IsStruct());
	auto type = target->GetType();
	for(auto i : member_name) {
		type = type.BaseType.StructType->GetMemberType(i);
	}

	return type;
}

std::set<SSASymbol *> SSAReadStructMemberStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSAReadStructMemberStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitReadStructMemberStatement(*this);
}
