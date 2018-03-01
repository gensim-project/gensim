#pragma once

#include <genC/ssa/statement/SSAStatement.h>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class FeatureDominanceAnalysis
			{
			public:
				bool HasDominatingSetFeature(uint32_t feature, const SSAStatement *reference_stmt) const;

			private:
				bool BlockHasDominatingSetFeature(uint32_t feature, const SSABlock *block, const SSAStatement *reference_stmt) const;
				bool StatementIsDominatingSetFeature(uint32_t feature, const SSAStatement *check_stmt) const;
			};
		}
	}
}
