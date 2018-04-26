/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   CallGraphAnalysis.h
 * Author: harry
 *
 * Created on 18 July 2017, 15:22
 */

#ifndef CALLGRAPHANALYSIS_H
#define CALLGRAPHANALYSIS_H

#include <set>
#include <map>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAActionBase;
			class SSAContext;
			class SSAFormAction;

			class CallGraph
			{
			public:
				typedef std::set<SSAActionBase *> callee_set_t;
				void AddCallee(SSAFormAction *caller, SSAActionBase *callee)
				{
					call_graph_[caller].insert(callee);
				}
				const callee_set_t &GetCallees(SSAFormAction *caller)
				{
					return call_graph_[caller];
				}
				
				const callee_set_t GetDeepCallees(SSAFormAction *caller);
			private:
				std::map<SSAFormAction*, std::set<SSAActionBase *>> call_graph_;
			};

			class CallGraphAnalysis
			{
			public:
				CallGraph Analyse(const SSAContext &ctx) const;

			private:
				CallGraph::callee_set_t GetCallees(SSAFormAction *action) const;
			};
		}
	}
}

#endif /* CALLGRAPHANALYSIS_H */

