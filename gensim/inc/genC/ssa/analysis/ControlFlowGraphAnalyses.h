/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   ControlFlowGraph.h
 * Author: harry
 *
 * Created on 14 June 2017, 16:29
 */

#ifndef CONTROLFLOWGRAPH_H
#define CONTROLFLOWGRAPH_H

#include <vector>
#include <map>
#include <list>

#include "genC/ssa/SSABlock.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{

			class SSAFormAction;
			class SSABlock;

			class SuccessorAnalysis
			{
			public:
				SuccessorAnalysis(const SSAFormAction *action);
				typedef SSABlock::BlockList successor_list_t;

				const successor_list_t &GetSuccessors(SSABlock *block) const
				{
					return successors_.at(block);
				}

			private:
				void BuildSuccessors(const SSAFormAction *action);
				std::map<SSABlock*, successor_list_t> successors_;
			};

			class PredecessorAnalysis
			{
			public:
				PredecessorAnalysis(const SSAFormAction *action);
				typedef std::list<SSABlock*> successor_list_t;

				const successor_list_t &GetPredecessors(SSABlock *block) const
				{
					return predecessors_.at(block);
				}

			private:
				void BuildPredecessors(const SSAFormAction *action);
				std::map<SSABlock*, successor_list_t> predecessors_;
			};

		}
	}
}

#endif /* CONTROLFLOWGRAPH_H */

