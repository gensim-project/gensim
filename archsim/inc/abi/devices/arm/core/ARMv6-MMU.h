/*
 * ARMv6-MMU.h
 *
 *  Created on: 12 Jan 2016
 *      Author: kuba
 */

#ifndef ARCSIM_INC_ABI_DEVICES_ARM_CORE_ARMV6_MMU_H_
#define ARCSIM_INC_ABI_DEVICES_ARM_CORE_ARMV6_MMU_H_

#include "abi/devices/MMU.h"
#include "abi/devices/arm/core/ArmControlCoprocessor.h"
#include "abi/memory/MemoryModel.h"

#include "gensim/gensim_processor.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"

// #include <devices/coco.h>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class ARMv6MMU : public MMU
			{
			public:

				TranslateResult Translate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t &phys_addr, const struct AccessInfo info, bool have_side_effects);

				TranslateResult TranslateRegion(gensim::Processor *cpu, uint32_t virt_addr, uint32_t size, uint32_t &phys_addr, const struct AccessInfo info, bool have_side_effects);

				const PageInfo GetInfo(uint32_t virt_addr);

				void FlushCaches();
				void Evict(virt_addr_t virt_addr);

			};

		}
	}
}


#endif /* ARCSIM_INC_ABI_DEVICES_ARM_CORE_ARMV6_MMU_H_ */

