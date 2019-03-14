/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/statement/SSACastStatement.h"
#include "genC/ssa/SSAStatementVisitor.h"

using namespace gensim::genc::ssa;

SSACastStatement::SSACastStatement(SSABlock* parent, const IRType& targetType, SSAStatement* expr, SSAStatement* before)
	: SSAStatement(Class_Unknown, 1, parent, before), target_type_(targetType), cast_type_(Cast_Unknown), option_(Option_None)
{
	SetExpr(expr);
}

SSACastStatement::CastType SSACastStatement::GetCastType() const
{
	if(cast_type_ == Cast_Unknown) {
		IRType::PromoteResult res = Expr()->GetType().AutoPromote(target_type_);

		switch (res) {
			case IRType::PROMOTE_TRUNCATE:
				cast_type_ = SSACastStatement::Cast_Truncate;
				break;
			case IRType::PROMOTE_OK:
				cast_type_ = SSACastStatement::Cast_ZeroExtend;
				break;
			case IRType::PROMOTE_OK_SIGNED:
				cast_type_ = SSACastStatement::Cast_SignExtend;
				break;
			case IRType::PROMOTE_SIGN_CHANGE: {
				if (Expr()->GetType().Signed) // If we are going from signed to unsigned, zero extend
					cast_type_ = SSACastStatement::Cast_ZeroExtend;
				else
					// otherwise, sign extend
					cast_type_ = SSACastStatement::Cast_SignExtend;
				break;
			}
			case IRType::PROMOTE_CONVERT: {
				cast_type_ = SSACastStatement::Cast_Convert;
				option_ = Option_RoundDefault;
				break;
			}
			case IRType::PROMOTE_VECTOR: {
				cast_type_ = SSACastStatement::Cast_VectorSplat;
				break;
			}
			default:
				UNEXPECTED;
		}
	}
	return cast_type_;
}

bool SSACastStatement::IsFixed() const
{
	return Expr()->IsFixed();
}

bool SSACastStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	success &= SSAStatement::Resolve(ctx);
	success &= Expr()->Resolve(ctx);

	if(GetCastType() == Cast_Reinterpret) {
		if(Expr()->GetType().SizeInBytes() != GetType().SizeInBytes()) {
			ctx.Error("Cannot bitcast between types of different sizes", GetDiag());
			success = false;
		}
	}

	Resolved = success;

	return success;
}

std::set<SSASymbol *> SSACastStatement::GetKilledVariables()
{
	return std::set<SSASymbol *>();
}

bool SSACastStatement::HasSideEffects() const
{
	return false;
}


SSACastStatement::~SSACastStatement()
{
}

void SSACastStatement::Accept(SSAStatementVisitor& visitor)
{
	visitor.VisitCastStatement(*this);
}
