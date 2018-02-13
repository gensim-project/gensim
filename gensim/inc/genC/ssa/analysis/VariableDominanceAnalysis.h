/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   VariableDominanceAnalysis,h.h
 * Author: harry
 *
 * Created on 04 August 2017, 10:03
 */

#ifndef VARIABLEDOMINANCEANALYSIS_H_H
#define VARIABLEDOMINANCEANALYSIS_H_H

#include "genC/ssa/analysis/SSADominance.h"

#include <map>
#include <set>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAVariableReadStatement;
			class SSAVariableWriteStatement;
			class SSASymbol;

			namespace analysis {
				class VariableDominanceInfo
				{
				public:
					typedef std::map<SSAVariableReadStatement*, std::set<SSAVariableWriteStatement*>> reads_dominated_by_writes_t;
					typedef std::map<SSAVariableWriteStatement*, std::set<SSAVariableReadStatement*>> writes_dominating_reads_t;

					reads_dominated_by_writes_t ReadsDominatedByWrites;
					writes_dominating_reads_t WritesDominatingReads;
				};

				class VariableDominanceAnalysis
				{
				public:
					VariableDominanceInfo Run(const SSASymbol *symbol) const;
					VariableDominanceInfo Run(const SSASymbol *symbol, const SSADominance &dom) const;
				};
			}
		}
	}
}

#endif /* VARIABLEDOMINANCEANALYSIS_H_H */

