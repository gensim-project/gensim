/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DominanceFrontierAnalysis.h
 * Author: harry
 *
 * Created on 13 October 2017, 13:56
 */

#ifndef DOMINANCEFRONTIERANALYSIS_H
#define DOMINANCEFRONTIERANALYSIS_H

#include <set>

namespace gensim {
	namespace genc {
		namespace ssa {
			class SSABlock;
	
			namespace analysis {
				class DominanceFrontierAnalysis {
				public:
					typedef std::set<SSABlock*> dominance_frontier_t;

					dominance_frontier_t GetDominanceFrontier(SSABlock *block) const;
				};
			}
			
		}
	}
}

#endif /* DOMINANCEFRONTIERANALYSIS_H */

