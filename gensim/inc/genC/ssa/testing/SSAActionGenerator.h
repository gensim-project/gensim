/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SSAGenerator.h
 * Author: harry
 *
 * Created on 22 April 2017, 14:24
 */

#ifndef SSAGENERATOR_H
#define SSAGENERATOR_H

#include <random>

namespace gensim
{

	namespace arch
	{
		class ArchDescription;
	}

	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
			class SSABlock;
			class SSAFormAction;

			namespace testing
			{

				class SSAActionGenerator
				{
				public:
					typedef std::mt19937_64 random_t;

					SSAActionGenerator(random_t &random, unsigned max_blocks, unsigned stmts_per_block, bool loops);

					SSAFormAction *Generate(SSAContext& context, const std::string &action_name);

				private:
					const unsigned _max_blocks;
					const unsigned _stmts_per_block;
					const bool _allow_loops;

					std::mt19937_64 &_random;

					SSABlock *generateBlock(SSAContext& context, bool must_return, SSAFormAction *action, std::vector<SSABlock*>& possible_target_blocks);
				};
			}
		}
	}
}

#endif /* SSAGENERATOR_H */

