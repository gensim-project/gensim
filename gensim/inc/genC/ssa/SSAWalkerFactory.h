#ifndef SSAWalkerFactory_H_
#define SSAWalkerFactory_H_

#include <map>

#include "Util.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAStatement;

			class SSAWalkerFactory;

			class SSANodeWalker
			{
			public:
				const SSAStatement &Statement;
				SSAWalkerFactory &Factory;

				SSANodeWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory);
				virtual ~SSANodeWalker();

				virtual bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const = 0;
				virtual bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const = 0;
				virtual std::string GetDynamicValue() const;
				virtual std::string GetFixedValue() const;
			};

			class SSAWalkerFactory
			{
			public:
				virtual ~SSAWalkerFactory();

				virtual SSANodeWalker *Create(const SSAStatement *statement) = 0;

				SSANodeWalker *GetOrCreate(const SSAStatement *statement);

			protected:
				void RegisterWalker(const SSAStatement *statement, SSANodeWalker *node);

			private:
				std::map<const SSAStatement *, SSANodeWalker *> _walkers;
			};
		}
	}
}

#endif
