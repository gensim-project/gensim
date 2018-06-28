/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/SSAType.h"
#include "DiagnosticContext.h"
#include "ComponentManager.h"

using gensim::DiagnosticContext;
using gensim::DiagnosticClass;

using namespace gensim::genc;
using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;



class ConstantValueSizeValidationPass : public SSAStatementValidationPass
{
	void VisitConstantStatement(SSAConstantStatement& stmt) override
	{
		bool constant_fits_type = SSAType::Cast(stmt.Constant, stmt.GetType(), stmt.GetType()) == stmt.Constant;
		Assert(constant_fits_type, "Constant value does not fit into specified type", stmt.GetDiag());
	}
};

class ConstantValueTypeValidationPass : public SSAStatementValidationPass
{
	void VisitConstantStatement(SSAConstantStatement& stmt) override
	{
		switch(stmt.Constant.Type()) {
			case IRConstant::Type_Integer:
				Assert(!stmt.GetType().IsFloating(), "Value type mismatch in constant", stmt.GetDiag());
				break;
			case IRConstant::Type_Float_Double:
			case IRConstant::Type_Float_Single:
				Assert(stmt.GetType().IsFloating(), "Value type mismatch in constant", stmt.GetDiag());
				break;

			case IRConstant::Type_Vector:
				Assert(stmt.GetType().VectorWidth > 1, "Value type mismatch in constant", stmt.GetDiag());
				break;
			default:
				UNEXPECTED;
		}
	}
};

RegisterComponent0(SSAStatementValidationPass, ConstantValueSizeValidationPass, "ConstantValueSize", "Check that constant values fit into their specified type");
RegisterComponent0(SSAStatementValidationPass, ConstantValueTypeValidationPass, "ConstantValueType", "Check that constant values match their specified type");
