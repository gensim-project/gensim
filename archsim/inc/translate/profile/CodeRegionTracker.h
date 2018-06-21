/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ProfileManager.h
 *
 *  Created on: 8 Sep 2015
 *      Author: harry
 */

#ifndef INC_TRANSLATE_PROFILE_PROFILEMANAGER_H_
#define INC_TRANSLATE_PROFILE_PROFILEMANAGER_H_

#include "abi/memory/MemoryModel.h"
#include "abi/Address.h"
#include "translate/profile/RegionArch.h"

#include "util/PubSubSync.h"
#include "util/LogContext.h"

UseLogContext(LogProfile);

#include <bitset>

namespace archsim
{
	namespace translate
	{
		namespace profile
		{

			/*
			 * This class is in charge of keeping track of which pages contain translated code, and generating invalidation events
			 * when these pages are invalidated.
			 */
			class CodeRegionTracker
			{
			public:
				CodeRegionTracker(archsim::util::PubSubContext &pubsub) : pubsub(pubsub)
				{
					Invalidate();
				}

				void MarkRegionAsCode(PhysicalAddress region_base)
				{
					code_regions.set(region_base.GetPageIndex());
					pubsub.Publish(PubSubType::RegionDispatchedForTranslationPhysical, (void*)(uint64_t)region_base.GetPageBase());
				}
				bool IsRegionCode(PhysicalAddress region_base)
				{
					return code_regions.test(region_base.GetPageIndex());
				}

				void InvalidateRegion(PhysicalAddress region_base)
				{
					if(IsRegionCode(region_base)) {
						LC_DEBUG1(LogProfile) << "Invalidating code region at " << std::hex << region_base.Get();
						pubsub.Publish(PubSubType::RegionInvalidatePhysical, (void*)(uint64_t)region_base.GetPageBase());
					}
					code_regions.reset(region_base.GetPageIndex());
				}
				void Invalidate()
				{
					code_regions.reset();
				}

			private:
				std::bitset<RegionArch::PageCount> code_regions;
				archsim::util::PubSubscriber pubsub;
			};

		}
	}
}


#endif /* INC_TRANSLATE_PROFILE_PROFILEMANAGER_H_ */
