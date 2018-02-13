#pragma once

#include <arch/ArchFeatures.h>
#include <genC/ssa/SSAFormAction.h>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class FeatureUseAnalysis
			{
			public:
				const std::set<const arch::ArchFeature *> GetUsedFeatures(const SSAFormAction *action) const;
			};
		}
	}
}
