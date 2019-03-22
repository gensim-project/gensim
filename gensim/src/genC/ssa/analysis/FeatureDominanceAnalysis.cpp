/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/analysis/FeatureDominanceAnalysis.h"
#include "genC/ssa/analysis/ControlFlowGraphAnalyses.h"
#include "genC/ssa/analysis/SSADominance.h"
#include "genC/ssa/statement/SSAIntrinsicStatement.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/SSABlock.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::analysis;

/**
 * Determine whether or not the feature identified by 'feature' is set by a statement that dominates 'reference_stmt'.
 * @param feature The feature ID to check for
 * @param reference_stmt The reference statement
 * @return True if 'stmt' has a dominating set feature of the given feature ID.
 */
bool FeatureDominanceAnalysis::HasDominatingSetFeature(uint32_t feature, const SSAStatement* reference_stmt) const
{
	SSADominance dominance_analysis;
	auto dominance = dominance_analysis.Calculate(reference_stmt->Parent->Parent);

	// Enumerate over all dominating blocks.  This list also includes the originating block.
	for (auto dominator : dominance[reference_stmt->Parent]) {
		// Check the block to see if it contains a dominating set-feature.
		if (BlockHasDominatingSetFeature(feature, dominator, reference_stmt)) {
			return true;
		}
	}

	// If we got here, then there were no dominating set-feature statements for the feature ID we're interested in.
	return false;
}

bool FeatureDominanceAnalysis::BlockHasDominatingSetFeature(uint32_t feature, const SSABlock* block, const SSAStatement* reference_statement) const
{
	// Enumerate over all statements in the block
	for (auto check_stmt : block->GetStatements()) {
		// If we discover the *reference* statement (i.e. the one which we are calculating dominating set-features
		// for, then stop processing this block, as we don't want to continue *after* the reference statement.
		if (check_stmt == reference_statement) break;

		// Check to see if this statement is a dominating set-feature.
		if (StatementIsDominatingSetFeature(feature, check_stmt)) {
			return true;
		}
	}

	// If we get here, then the block does not contain a dominating set-feature statement.
	return false;
}

bool FeatureDominanceAnalysis::StatementIsDominatingSetFeature(uint32_t feature, const SSAStatement* check_stmt) const
{
	// Try casting the statement to an intrinsic...
	if (auto check_intrinsic = dynamic_cast<const SSAIntrinsicStatement *>(check_stmt)) {
		// And, check to see if the intrinsic is a set-feature...
		if (check_intrinsic->GetID() == IntrinsicID::SetFeature) {
			// And, extract the feature id being set by this statement...
			auto set_stmt_feature_id = dynamic_cast<const SSAConstantStatement *>(check_intrinsic->Args(0));

			// We're going to assume that it should ALWAYS be a constant statement.
			assert(set_stmt_feature_id);

			// If the feature ID of this set-feature intrinsic matches the feature ID we're looking for, then
			// immediately return true, since there is at least one dominating set-feature of this feature id.
			if (set_stmt_feature_id->Constant.Int() == feature) {
				return true;
			}
		}
	}

	// If we get here then the statement is not a dominating set-feature.
	return false;
}
