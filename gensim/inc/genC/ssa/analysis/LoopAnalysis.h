/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   LoopAnalysis.h
 * Author: harry
 *
 * Created on 02 August 2017, 12:48
 */

#ifndef LOOPANALYSIS_H
#define LOOPANALYSIS_H

#include <set>
#include <vector>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSABlock;
			class SSAFormAction;

			class LoopAnalysisResult
			{
			public:
				typedef std::set<SSABlock *> loop_blocks_t;
				typedef std::vector<std::set<SSABlock*>> loop_list_t;

				// For now, just determine if a loop exists in the action
				bool LoopExists;
			};

			class LoopAnalysis
			{
			public:
				LoopAnalysisResult Analyse(const SSAFormAction &action);
			};
		}
	}
}

#endif /* LOOPANALYSIS_H */

