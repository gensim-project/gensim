/*
 * Analysis.h
 *
 *  Created on: 22 Oct 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_ANALYSES_ANALYSIS_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_ANALYSES_ANALYSIS_H_

#include "blockjit/translation-context.h"

#include <map>

namespace captive
{
	namespace shared
	{
		class IRInstruction;
	}
	namespace arch
	{
		namespace jit
		{

			namespace analyses
			{

				template<typename ReturnType> class Analysis
				{
				public:
					ReturnType Run(TranslationContext &ctx);
				};

				class HostRegLivenessData
				{
				public:
					std::map<const shared::IRInstruction*, uint32_t> live_out;
				};

				class HostRegLivenessAnalysis : public Analysis<HostRegLivenessData>
				{
				public:
					HostRegLivenessData Run(TranslationContext &ctx);
				};

			}
		}
	}
}

#endif /* INC_BLOCKJIT_BLOCK_COMPILER_ANALYSES_ANALYSIS_H_ */
