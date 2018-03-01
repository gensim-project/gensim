/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSACastStatement.h"
#include "genC/ssa/SSAType.h"
#include "DiagnosticContext.h"
#include "ComponentManager.h"

using gensim::DiagnosticContext;
using gensim::DiagnosticClass;

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;


class CastToVoidValidationPass : public SSAStatementValidationPass
{
public:
	void VisitCastStatement(SSACastStatement& stmt) override
	{
		if(stmt.GetType() == gensim::genc::IRTypes::Void) {
			Fail("Cannot cast value to void", stmt.GetDiag());
		}
	}
};

class CastToReferenceValidationPass : public SSAStatementValidationPass
{
public:
	void VisitCastStatement(SSACastStatement& stmt) override
	{
		if(stmt.GetType().Reference) {
			Fail("Cannot cast value to reference", stmt.GetDiag());
		}
	}
};

RegisterComponent0(SSAStatementValidationPass, CastToVoidValidationPass, "CastToVoid", "Check that values are not cast to void");
RegisterComponent0(SSAStatementValidationPass, CastToReferenceValidationPass, "CastToReference", "Check that values are not cast to references");