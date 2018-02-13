/*
 * PathBasedTestGenerator.h
 *
 *  Created on: 16 Oct 2013
 *      Author: harry
 */

#ifndef PATHBASEDTESTGENERATOR_H_
#define PATHBASEDTESTGENERATOR_H_

#include "generators/GenerationManager.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

#include <CoverageFile.h>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAFormAction;
		}
	}
	namespace generator
	{
		namespace testgeneration
		{

			namespace ga
			{
				class GAConfiguration;
			}

			class ValueSet;
			class Constraint;
			class ConstraintSet;

			class PathBasedTestGenerator : public GenerationComponent
			{
			public:
				PathBasedTestGenerator(GenerationManager &man);
				virtual ~PathBasedTestGenerator();

				virtual bool Generate() const;
				virtual std::string GetFunction() const;
				virtual const std::vector<std::string> GetSources() const;

				std::vector<ConstraintSet*> ExpandConstraints(const genc::ssa::SSAFormAction *action, const coverage::CoveragePath &path, ConstraintSet *original) const;

				std::unordered_set<ValueSet> ConstrainPath(const coverage::CVGDescription *cvgdesc, const coverage::CoveragePath &Path) const;

			private:
				bool GetConstraintsFor(const genc::ssa::SSAFormAction &action, const coverage::CoveragePath &path, ConstraintSet *&) const;

				mutable std::mutex _stats_mutex;

				struct test_info {
				public:
					uint32_t tests_generated, tests_conflict, tests_could_not_generate;
				};
				mutable std::map<uint32_t, struct test_info> _test_stats;
				mutable std::atomic<uint32_t> tests_generated, tests_could_not_generate, tests_conflict, tests_unsupported;
			};
		}
	}
}

#endif /* PATHBASEDTESTGENERATOR_H_ */
