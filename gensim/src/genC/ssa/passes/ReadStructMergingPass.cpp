/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/analysis/VariableUseDefAnalysis.h"


using namespace gensim::genc::ssa;

class ReadStructMergingPass : public SSAPass
{
public:
	virtual bool Run(SSAFormAction& action) const
	{
		bool changed = false;

		analysis::VariableUseDefAnalysis vuda;
		auto vudi = vuda.Run(&action);

		for(auto i : action.GetBlocks()) {
			for(auto stmt : i->GetStatements()) {

				SSAReadStructMemberStatement *rdstruct = dynamic_cast<SSAReadStructMemberStatement*>(stmt);
				if(rdstruct != nullptr) {
					changed |= RunOnStatement(rdstruct, vudi);
				}

			}
		}
		return changed;
	}

private:
	bool RunOnStatement(SSAReadStructMemberStatement* stmt, analysis::ActionUseDefInfo &vudi) const
	{
		// If the target of this read struct is a variable which has only had the value of a read struct written to it, then we can merge the statements.
		SSASymbol *target = stmt->Target();
		if(vudi.Get(target).Defs().size() != 1) {
			return false;
		}

		auto def = vudi.Get(target).Defs().front();
		// need to make sure that the def is a write variable statement
		SSAVariableWriteStatement *def_write = dynamic_cast<SSAVariableWriteStatement*>(def);
		if(def_write == nullptr) {
			return false;
		}

		// is the expr of the def a read struct member statement?
		auto def_expr = def_write->Expr();
		auto def_expr_rdstruct = dynamic_cast<SSAReadStructMemberStatement *>(def_expr);
		if(def_expr_rdstruct == nullptr) {
			return false;
		}

		// we've established that the original statement's source struct was read from another struct. So, we can merge the writes.
		return MergeWrites(stmt, def_expr_rdstruct);
	}

	bool MergeWrites(SSAReadStructMemberStatement *mergee, SSAReadStructMemberStatement *base) const
	{
		mergee->SetTarget(base->Target());
		mergee->MemberNames.insert(mergee->MemberNames.begin(), base->MemberNames.begin(), base->MemberNames.end());
		return true;
	}

};

RegisterComponent0(SSAPass, ReadStructMergingPass, "ReadStructMerge", "Merge Struct Reads");
