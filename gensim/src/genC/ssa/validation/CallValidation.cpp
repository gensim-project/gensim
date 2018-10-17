/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSAType.h"
#include "ComponentManager.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

class CallValidationPass : public SSAStatementValidationPass
{
public:
	void VisitCallStatement(SSACallStatement& stmt) override
	{
		// check that call parameters match call
		unsigned arg_count = stmt.ArgCount();
		unsigned expected_arg_count = stmt.Target()->GetPrototype().ParameterTypes().size();
		if(arg_count != expected_arg_count) {
			Fail("Expected " + std::to_string(expected_arg_count) + " arguments, got " + std::to_string(arg_count), stmt.GetDiag());
			return;
		}

		for(unsigned i = 0; i < arg_count; ++i) {
			auto &arg_type = stmt.Target()->GetPrototype().ParameterTypes().at(i);
			auto arg = stmt.Arg(i);

			if(arg_type.Reference) {
				SSASymbol *sym = dynamic_cast<SSASymbol*>(arg);
				if(sym != nullptr) {
					Assert(sym->GetType().IsStruct() || (sym->GetType() == SSAType::Ref(arg_type)), "Parameter " + std::to_string(i+1) + ": Symbol of incorrect type passed as reference parameter", stmt.GetDiag());
				} else {
					Fail("Parameter " + std::to_string(i+1) + ": Expected a reference", stmt.GetDiag());
				}
			} else {
				Assert(arg->GetType() == arg_type, "Parameter " + std::to_string(i+1) + ": Value of incorrect type passed as parameter", stmt.GetDiag());
				Assert(((SSAStatement*)arg)->Parent == stmt.Parent, "Parameter " + std::to_string(i+1) + ": Parameter value must be in same block as call", stmt.GetDiag());
			}
		}
	}
};

RegisterComponent0(SSAStatementValidationPass, CallValidationPass, "CallValidation", "Check call parameter types")
