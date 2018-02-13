/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SSADominance.h
 * Author: harry
 *
 * Created on 18 April 2017, 15:24
 */

#ifndef SSADOMINANCE_H
#define SSADOMINANCE_H

#include <map>
#include <set>

#include "util/VSet.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSABlock;
			class SSAFormAction;

			namespace analysis {
				class SSADominance
				{
				public:
					typedef std::set<const SSABlock*> block_dominators_t;

					/*
					 * The key is a block
					 * The value is the set of blocks which dominate the key
					 */
					typedef std::map<const SSABlock *, block_dominators_t> dominance_info_t;

					dominance_info_t Calculate(const SSAFormAction *action);

					std::string Print(const dominance_info_t &dominance) const;
				};
			}
		}
	}
}

#endif /* SSADOMINANCE_H */

