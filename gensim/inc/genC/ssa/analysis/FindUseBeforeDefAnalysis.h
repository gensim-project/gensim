/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   FindUseBeforeDefAnalysis.h
 * Author: harry
 *
 * Created on 04 August 2017, 10:23
 */

#ifndef FINDUSEBEFOREDEFANALYSIS_H
#define FINDUSEBEFOREDEFANALYSIS_H

#include <set>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAStatement;
			class SSAFormAction;

			class FindUseBeforeDefAnalysis
			{
			public:
				typedef std::set<SSAStatement*> use_before_def_list_t;
				use_before_def_list_t FindUsesBeforeDefs(const SSAFormAction *action);

			private:

			};
		}
	}
}

#endif /* FINDUSEBEFOREDEFANALYSIS_H */

