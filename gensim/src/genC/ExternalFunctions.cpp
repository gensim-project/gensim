#include "define.h"
#include "genC/Intrinsics.h"
#include "genC/Parser.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRSignature.h"
#include "genC/ir/IR.h"
#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

void GenCContext::AddExternalFunction(const std::string& name, const IRType& retty, const IRSignature::param_type_list_t& ptl)
{
	if (ExternalTable.count(name) != 0) {
		throw std::logic_error("External function with the name '" + name + "' is already registered.");
	}

	IRSignature sig(name, retty, ptl);
	sig.AddAttribute(gensim::genc::ActionAttribute::External);
	ExternalTable[name] = new IRExternalAction(sig, *this);
}

static const IRType &WordType(const arch::ArchDescription *arch)
{
	switch(arch->wordsize) {
		case 32:
			return IRTypes::UInt32;
		case 64:
			return IRTypes::UInt64;
		default:
			UNIMPLEMENTED;
	}
}

void GenCContext::LoadExternalFunctions()
{
	const IRType &wordtype = WordType(&Arch);

	AddExternalFunction("__builtin_external_call", IRTypes::Void, {});

	// TLB Maintenance
	AddExternalFunction("flush", IRTypes::Void, {});
	AddExternalFunction("flush_dtlb", IRTypes::Void, {});
	AddExternalFunction("flush_itlb", IRTypes::Void, {});
	AddExternalFunction("flush_dtlb_entry", IRTypes::Void, {IRParam("addr", wordtype)});
	AddExternalFunction("flush_itlb_entry", IRTypes::Void, {IRParam("addr", wordtype)});
	
	AddExternalFunction("mmu_flush_all", IRTypes::Void, {});
	AddExternalFunction("mmu_flush_va", IRTypes::Void, {IRParam("addr", wordtype)});
	AddExternalFunction("mmu_notify_asid_change", IRTypes::Void, {IRParam("asid", IRTypes::UInt32)});
	AddExternalFunction("mmu_notify_pgt_change", IRTypes::Void, {});
	
	// Floating-point
	AddExternalFunction("__builtin_clear_fpex", IRTypes::Void, {});
	AddExternalFunction("__builtin_fpex_invalid", IRTypes::UInt8, {});
	AddExternalFunction("__builtin_fpex_underflow", IRTypes::UInt8, {});
	AddExternalFunction("__builtin_fpex_overflow", IRTypes::UInt8, {});
	
	AddExternalFunction("__builtin_f32_round", IRTypes::Float, {IRParam("value", IRTypes::Float), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_f64_round", IRTypes::Double, {IRParam("value", IRTypes::Double), IRParam("rmode", IRTypes::UInt8)});
	
	AddExternalFunction("__builtin_f32_is_nan", IRTypes::UInt8, {IRParam("value", IRTypes::Float)});
	AddExternalFunction("__builtin_f32_is_snan", IRTypes::UInt8, {IRParam("value", IRTypes::Float)});
	AddExternalFunction("__builtin_f32_is_qnan", IRTypes::UInt8, {IRParam("value", IRTypes::Float)});
	AddExternalFunction("__builtin_f64_is_nan", IRTypes::UInt8, {IRParam("value", IRTypes::Double)});
	AddExternalFunction("__builtin_f64_is_snan", IRTypes::UInt8, {IRParam("value", IRTypes::Double)});
	AddExternalFunction("__builtin_f64_is_qnan", IRTypes::UInt8, {IRParam("value", IRTypes::Double)});
	
	AddExternalFunction("__builtin_fcvt_f32_s32", IRTypes::Int32, {IRParam("value", IRTypes::Float), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f64_s32", IRTypes::Int32, {IRParam("value", IRTypes::Double), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f32_s64", IRTypes::Int64, {IRParam("value", IRTypes::Float), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f64_s64", IRTypes::Int64, {IRParam("value", IRTypes::Double), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f32_u32", IRTypes::UInt32, {IRParam("value", IRTypes::Float), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f64_u32", IRTypes::UInt32, {IRParam("value", IRTypes::Double), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f32_u64", IRTypes::UInt64, {IRParam("value", IRTypes::Float), IRParam("rmode", IRTypes::UInt8)});
	AddExternalFunction("__builtin_fcvt_f64_u64", IRTypes::UInt64, {IRParam("value", IRTypes::Double), IRParam("rmode", IRTypes::UInt8)});

	AddExternalFunction("__builtin_cmpf32_flags", IRTypes::Void, {IRParam("a", IRTypes::Float), IRParam("b", IRTypes::Float)});
	AddExternalFunction("__builtin_cmpf32e_flags", IRTypes::Void, {IRParam("a", IRTypes::Float), IRParam("b", IRTypes::Float)});
	AddExternalFunction("__builtin_cmpf64_flags", IRTypes::Void, {IRParam("a", IRTypes::Double), IRParam("b", IRTypes::Double)});
	AddExternalFunction("__builtin_cmpf64e_flags", IRTypes::Void, {IRParam("a", IRTypes::Double), IRParam("b", IRTypes::Double)});
	
	AddExternalFunction("__builtin_polymul8", IRTypes::UInt16, {IRParam("a", IRTypes::UInt8), IRParam("b", IRTypes::UInt8)});
	AddExternalFunction("__builtin_polymul64", IRTypes::UInt128, {IRParam("a", IRTypes::UInt64), IRParam("b", IRTypes::UInt64)});
}

SSAStatement *IRCallExpression::EmitExternalCall(SSABuilder &bldr, const gensim::arch::ArchDescription &Arch) const
{
	auto target = dynamic_cast<IRExternalAction *>(Target);
	if (target == nullptr) {
		throw std::logic_error("Target of external call is not an IRExternalAction");
	}

	SSAActionPrototype prototype(target->GetSignature());
	SSAExternalAction *target_action = nullptr;
	if(!bldr.Context.HasAction(prototype.GetIRSignature().GetName())) {
		target_action = new SSAExternalAction(bldr.Context, prototype);
		bldr.Context.AddAction(target_action);
	} else {
		target_action = static_cast<SSAExternalAction*>(bldr.Context.GetAction(prototype.GetIRSignature().GetName()));
	}

	std::vector<SSAValue *> arg_statements;

	auto params = target->GetSignature().GetParams();
	if (params.size() != Args.size()) throw std::logic_error("Argument/Parameter count mismatch when emitting external call '" + target->GetSignature().GetName() + "'");

	int arg_index = 0;
	for (const auto& arg : Args) {
		SSAStatement *arg_statement = arg->EmitSSAForm(bldr);

		auto param = params.at(arg_index++);
		if (arg_statement->GetType() != param.GetType()) {
			const auto& dn = arg_statement->GetDiag();
			arg_statement = new SSACastStatement(&bldr.GetBlock(), param.GetType(), arg_statement);
			arg_statement->SetDiag(dn);
		}

		arg_statements.push_back(arg_statement);
	}

	SSACallStatement *stmt = new SSACallStatement(&bldr.GetBlock(), target_action, arg_statements);
	stmt->SetDiag(Diag());

	return stmt;
}