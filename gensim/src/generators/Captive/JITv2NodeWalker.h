/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/SSAWalkerFactory.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class RegisterAllocation;
		}
	}

	namespace generator
	{
		namespace captive
		{
			class JITv2NodeWalkerFactory : public genc::ssa::SSAWalkerFactory
			{
			public:
				JITv2NodeWalkerFactory(const genc::ssa::RegisterAllocation& ra) : _ra(ra) { }

				genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement) override;

				const genc::ssa::RegisterAllocation& RegisterAllocation() const
				{
					return _ra;
				}

			private:
				const genc::ssa::RegisterAllocation& _ra;
			};
		}
	}
}
