/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ReachabilityAnalysis.h
 * Author: harry
 *
 * Created on 14 August 2017, 16:58
 */

#ifndef REACHABILITYANALYSIS_H
#define REACHABILITYANALYSIS_H

#include <set>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSABlock;
			class SSAFormAction;

			class ReachabilityAnalysis
			{
			public:
				std::set<SSABlock*> GetReachable(const SSAFormAction &action);
				std::set<SSABlock*> GetReachable(const SSAFormAction &action, const std::set<SSABlock*> &reachability_sources);
			};

		}
	}
}


#endif /* REACHABILITYANALYSIS_H */

