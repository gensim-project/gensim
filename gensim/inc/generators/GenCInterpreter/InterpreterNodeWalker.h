/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef INTERPRETERNODEWALKER_H_
#define INTERPRETERNODEWALKER_H_

#include "genC/ssa/SSAWalkerFactory.h"

namespace gensim
{

	class GenCInterpreterNodeFactory : public genc::ssa::SSAWalkerFactory
	{
	public:
		virtual genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement);

	};

}
#endif /* INTERPRETERNODEWALKER_H_ */
