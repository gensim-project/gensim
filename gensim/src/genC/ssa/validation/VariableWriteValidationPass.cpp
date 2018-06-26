/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/validation/SSAActionValidationPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/SSAType.h"
#include "DiagnosticContext.h"
#include "ComponentManager.h"

using gensim::DiagnosticContext;
using gensim::DiagnosticClass;

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

class VariableWriteValidationPass : public SSAActionValidationPass
{
	bool Run(const SSAFormAction* action, DiagnosticContext& ctx) override
	{
		bool success = true;

		for(auto block : action->GetBlocks()) {
			for(auto stmt : block->GetStatements()) {
				if(auto write = dynamic_cast<SSAVariableWriteStatement*>(stmt)) {
					const SSAType &variable_type = write->Target()->GetType();
					const SSAType &expr_type = write->Expr()->GetType();

					if(variable_type != expr_type) {
						ctx.AddEntry(DiagnosticClass::Error, "Value written to variable does not match variable type", write->GetDiag());
						success = false;
					}
				}
			}
		}

		return success;
	}
};

RegisterComponent0(SSAActionValidationPass, VariableWriteValidationPass, "VariableWrite", "Check that the values written to variables have the correct type");
