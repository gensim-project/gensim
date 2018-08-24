/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <genC/ssa/analysis/FeatureUseAnalysis.h>
#include <genC/ssa/statement/SSAIntrinsicStatement.h>
#include <genC/ssa/statement/SSAConstantStatement.h>
#include <arch/ArchDescription.h>

using namespace gensim::arch;
using namespace gensim::genc::ssa;

const std::set<const ArchFeature *> FeatureUseAnalysis::GetUsedFeatures(const SSAFormAction* action) const
{
	std::set<const ArchFeature *> feature_set;

	for (auto block : action->GetBlocks()) {
		for (auto stmt : block->GetStatements()) {
			if (auto intrinsic = dynamic_cast<SSAIntrinsicStatement *>(stmt)) {
				if (intrinsic->Type == SSAIntrinsicStatement::SSAIntrinsic_GetFeature || intrinsic->Type == SSAIntrinsicStatement::SSAIntrinsic_SetFeature) {
					auto feature_index_arg = dynamic_cast<SSAConstantStatement *>(intrinsic->Args(0));
					if (feature_index_arg != nullptr && feature_index_arg->Constant.Type() == IRConstant::Type_Integer) {
						auto feature = action->Arch->GetFeatures().GetFeature((uint32_t)feature_index_arg->Constant.Int());
						if (feature != nullptr) {
							feature_set.insert(feature);
						}
					}
				}
			}
		}
	}

	return feature_set;
}
