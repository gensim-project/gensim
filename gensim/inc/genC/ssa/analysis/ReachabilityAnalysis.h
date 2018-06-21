/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

