/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/analysis/SSADominance.h"
#include "genC/ssa/validation/SSAActionValidationPass.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/SSAFormAction.h"
#include "ComponentManager.h"

using namespace gensim;
using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::analysis;
using namespace gensim::genc::ssa::validation;

/*
 * Build a CFG of the action, and then use it to validate all of the phi nodes
 * in the action
 */
class PhiNodeValidationPass : public SSAActionValidationPass
{
public:
	bool Run(const SSAFormAction* action, DiagnosticContext& ctx) override
	{
		auto phi_statements = GetPhiStatements(action);
		return CheckTypes(phi_statements, action, ctx) && CheckDominators(phi_statements, action, ctx);
	}

private:
	bool CheckTypes(const std::vector<SSAPhiStatement*> &phi_statements, const SSAFormAction* action, DiagnosticContext& ctx)
	{
		bool success = true;

		for(auto stmt : phi_statements) {
			for(auto member : stmt->Get()) {
				if(member->GetType() != stmt->GetType()) {
					ctx.Error("Phi statement types must be consistent", stmt->GetDiag());
					success = false;
				}
			}
		}
		return success;
	}

	bool CheckDominators(const std::vector<SSAPhiStatement*> &phi_statements, const SSAFormAction *action, DiagnosticContext &ctx)
	{
		SSADominance dominance;
		auto info = dominance.Calculate(action);
		bool success = true;

		for(auto phi : phi_statements) {
			const SSABlock *phi_parent = phi->Parent;
			for(auto member : phi->Get()) {
				const SSABlock *member_parent = ((SSAStatement*)member)->Parent;

				if(!info.at(phi_parent).count(member_parent)) {
					ctx.Error("Phi statement must be dominated by all members", phi->GetDiag());
					success = false;
				}
			}
		}

		return success;
	}

	std::vector<SSAPhiStatement*> GetPhiStatements(const SSAFormAction *action)
	{
		std::vector<SSAPhiStatement*> output;
		for(auto block : action->Blocks) {
			for(auto stmt : block->GetStatements()) {
				if(dynamic_cast<SSAPhiStatement*>(stmt)) {
					output.push_back((SSAPhiStatement*)stmt);
				}
			}
		}
		return output;
	}

};

RegisterComponent0(SSAActionValidationPass, PhiNodeValidationPass, "PhiNodeValidationPass", "Check types and dominance for phi nodes")
