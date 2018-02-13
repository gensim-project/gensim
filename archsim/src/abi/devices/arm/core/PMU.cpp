#include "abi/devices/arm/core/PMU.h"
#include "abi/devices/Device.h"
#include "abi/devices/PeripheralManager.h"
#include "gensim/gensim_processor.h"

using namespace archsim::abi::devices::arm::core;

std::map<int, archsim::util::Counter64 *> pmu_events;

PMU::PMU()
{
	ctlreg = 0;
	cur_ctr = 0;

	for (int i = 0; i < MAX_PMU_COUNTERS; i++) {
		pmu_ctrs[i] = NULL;
	}
}

bool PMU::PMCR(bool rd, uint32_t& data)
{
	if (rd) {
		data = ctlreg;
	} else {
		ctlreg = data;

		if ((data & 2) == 2) {
			// Reset Counters
			for (int i = 0; i < MAX_PMU_COUNTERS; i++) {
				if (pmu_ctrs[i]) {
					pmu_ctrs[i]->clear();
				}
			}
		}
	}

	return true;
}

bool PMU::PMSELR(bool rd, uint32_t& data)
{
	if (rd) {
		data = cur_ctr;
	} else {
		if (data >= MAX_PMU_COUNTERS) {
			cur_ctr = 0;
		} else {
			cur_ctr = data;
		}
	}

	return true;
}

bool PMU::PMXEVTYPER(bool rd, uint32_t& data)
{
	if (rd) {
		return false;
	} else {
		pmu_ctrs[cur_ctr] = pmu_events[data];
		return true;
	}
}

bool PMU::PMCNTENSET(bool rd, uint32_t& data)
{
	return true;
}

bool PMU::PMCNTENCLR(bool rd, uint32_t& data)
{
	return true;
}

bool PMU::CCNTVAL(bool rd, uint32_t& data)
{
	if (rd) {
		data = 0;
		return true;
	} else {
		return false;
	}
}

bool PMU::PMNxVAL(bool rd, uint32_t& data)
{
	if (rd) {
		if (pmu_ctrs[cur_ctr] != NULL) {
			data = (uint32_t)pmu_ctrs[cur_ctr]->get_value();
		} else {
			data = 0;
		}

		return true;
	} else {
		return false;
	}
}
