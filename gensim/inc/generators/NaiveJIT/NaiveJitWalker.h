/*
 * InterpreterNodeWalker.h
 *
 *  Created on: 24 Mar 2015
 *      Author: harry
 */

#ifndef INTERPRETERNODEWALKER_H_
#define INTERPRETERNODEWALKER_H_

#include "genC/ssa/SSAWalkerFactory.h"

namespace gensim
{
	namespace generator
	{
		namespace naivejit
		{

			class NaiveJitNodeFactory : public genc::ssa::SSAWalkerFactory
			{
			public:
				virtual genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement);

			};

		}
	}
}
#endif /* INTERPRETERNODEWALKER_H_ */
