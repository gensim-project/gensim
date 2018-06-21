/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

