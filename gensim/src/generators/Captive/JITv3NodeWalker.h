/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/SSAWalkerFactory.h"

namespace gensim
{
	namespace generator
	{
		namespace captive
		{

			class JITv3NodeWalkerFactory : public genc::ssa::SSAWalkerFactory
			{
			public:

				genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement) override;
			};
		}
	}
}
