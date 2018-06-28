/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			SSANodeWalker::SSANodeWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : Statement(_statement), Factory(_factory) {}
			SSANodeWalker::~SSANodeWalker() {}

			std::string SSANodeWalker::GetFixedValue() const
			{
				return Statement.GetName();
			}

			std::string SSANodeWalker::GetDynamicValue() const
			{
				return Statement.GetName();
			}

			SSAWalkerFactory::~SSAWalkerFactory() {}
			void SSAWalkerFactory::RegisterWalker(const SSAStatement *statement, SSANodeWalker *walker)
			{
				_walkers[statement] = walker;
			}

			SSANodeWalker *SSAWalkerFactory::GetOrCreate(const SSAStatement *statement)
			{
				if (_walkers.find(statement) != _walkers.end()) return _walkers[statement];
				SSANodeWalker *walker = Create(statement);
				_walkers[statement] = walker;
				return walker;
			}
		}
	}
}
