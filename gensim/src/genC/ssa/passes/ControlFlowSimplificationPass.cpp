/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAIfStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/statement/SSASwitchStatement.h"
#include "genC/ssa/statement/SSAStatement.h"

using namespace gensim::genc::ssa;

/**
 * Simplify three types of control flow statement:
 *  - if an IF statement has the same true and false target, replace it with a jump
 *  - if an IF statement has a constant expression, replace it with a jump
 *  - if a SWITCH statement has a constant expression, replace it with a jump
 */

static bool TrySimplifyIf(SSAIfStatement *if_stmt, SSABlock *block)
{
	if(if_stmt->TrueTarget() == if_stmt->FalseTarget()) {
//			fprintf(stderr, "*** OPPORTUNITY SAME IF!\n");
		// this doesn't happen often so just ignore it for now
	}

	if(dynamic_cast<SSAConstantStatement*>(if_stmt->Expr())) {
		SSAConstantStatement *constant_stmt = (SSAConstantStatement*)if_stmt->Expr();
		bool is_true = false;
		switch(constant_stmt->Constant.Type()) {
			case gensim::genc::IRConstant::Type_Integer:
				is_true = constant_stmt->Constant.Int() != 0;
				break;
			case gensim::genc::IRConstant::Type_Float_Single:
				is_true = constant_stmt->Constant.Flt() != 0;
				break;
			case gensim::genc::IRConstant::Type_Float_Double:
				is_true = constant_stmt->Constant.Dbl() != 0;
				break;
			default:
				throw std::logic_error("");
		}

		SSABlock *real_target;
		if(is_true) {
			real_target = if_stmt->TrueTarget();
		} else {
			real_target = if_stmt->FalseTarget();
		}

		auto jump_stmt = new SSAJumpStatement(block, *real_target, if_stmt);
		jump_stmt->SetDiag(if_stmt->GetDiag());

		block->RemoveStatement(*if_stmt);
		if_stmt->Dispose();
		delete if_stmt;
		return true;
	}

	return false;
}

static bool TrySimplifySwitch(SSASwitchStatement *switch_stmt, SSABlock *block)
{
	if(dynamic_cast<SSAConstantStatement*>(switch_stmt->Expr())) {
		SSAConstantStatement *expr = (SSAConstantStatement*)switch_stmt->Expr();

		// Find the case statement associated with the given expression
		SSABlock *target = switch_stmt->Default();
		for(auto caze : switch_stmt->GetValues()) {
			SSAConstantStatement *cazevalue = dynamic_cast<SSAConstantStatement*>(caze.first);
			if(cazevalue == nullptr) {
				throw std::logic_error("");
			}

			if(cazevalue->Constant == expr->Constant) {
				target = caze.second;
			}
		}

		// Replace the switch statement with a jump
		SSAJumpStatement *jump = new SSAJumpStatement(block, *target, switch_stmt);

		block->RemoveStatement(*switch_stmt);
		switch_stmt->Dispose();
		delete switch_stmt;

		return true;
	}
	return false;
}

class ControlFlowSimplificationPass : public SSAPass
{
public:
	virtual ~ControlFlowSimplificationPass()
	{

	}

	bool Run(SSAFormAction& action)const override
	{
		bool changed = false;
		for(auto block : action.GetBlocks()) {
			SSAControlFlowStatement *ctrlflow = block->GetControlFlow();

			if(dynamic_cast<SSAIfStatement*>(ctrlflow)) {
				SSAIfStatement *if_stmt = (SSAIfStatement*)ctrlflow;

				changed |= TrySimplifyIf(if_stmt, block);
			} else if(dynamic_cast<SSASwitchStatement*>(ctrlflow)) {
				SSASwitchStatement *switch_stmt = (SSASwitchStatement*)ctrlflow;

				changed |= TrySimplifySwitch(switch_stmt, block);
			}
		}

		return changed;
	}
};

RegisterComponent0(SSAPass, ControlFlowSimplificationPass, "ControlFlowSimplification", "Simplify some types of control flow statement")
