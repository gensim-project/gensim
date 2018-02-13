/*
 * File:   PMU.h
 * Author: s0457958
 *
 * Created on 06 January 2015, 11:04
 */

#ifndef PMU_H
#define	PMU_H

#include "define.h"
#include "util/Counter.h"

#include <map>

#define MAX_PMU_COUNTERS	6

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class Device;

			namespace arm
			{
				namespace core
				{
					class PMU
					{
					public:
						PMU();

						bool PMCR(bool rd, uint32_t& data);
						bool PMSELR(bool rd, uint32_t& data);
						bool PMXEVTYPER(bool rd, uint32_t& data);
						bool PMCNTENSET(bool rd, uint32_t& data);
						bool PMCNTENCLR(bool rd, uint32_t& data);
						bool CCNTVAL(bool rd, uint32_t& data);
						bool PMNxVAL(bool rd, uint32_t& data);

					private:
						uint32_t ctlreg;
						int cur_ctr;
						util::Counter64 *pmu_ctrs[MAX_PMU_COUNTERS];
					};
				}
			}
		}
	}
}

#endif	/* PMU_H */

