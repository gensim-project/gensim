/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/statement/SSAReadStructMemberStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ir/IR.h"
#include "genC/Parser.h"

#include "isa/InstructionDescription.h"
#include "isa/ISADescription.h"

using namespace gensim::genc::ssa;
using gensim::genc::IRType;

SSAReadStructMemberStatement::SSAReadStructMemberStatement(SSABlock *parent, SSASymbol *target, std::string member, int32_t idx, SSAStatement *before)
	: SSAStatement(Class_Unknown, 1, parent, before), MemberName(member), Index(idx)
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

	bool found_member = false;
	// look for correct member in struct
	for(auto i : struct_type.BaseType.StructType->Members) {
		if(i.Name == this->MemberName) {
			found_member = true;
		}
	}

	if(!found_member) {
		ctx.Error("Instruction does not have field " + MemberName, GetDiag());
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

SSAType SSAReadStructMemberStatement::ResolveType(const SSASymbol *target, const std::string& member_name, int index)
{
	GASSERT(target->GetType().IsStruct());
	return target->GetType().BaseType.StructType->GetMemberType(member_name);
}

std::set<SSASymbol *> SSAReadStructMemberStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

void SSAReadStructMemberStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitReadStructMemberStatement(*this);
}
