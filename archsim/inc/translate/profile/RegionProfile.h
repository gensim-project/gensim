/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/* 
 * File:   RegionProfile.h
 * Author: harry
 *
 * Created on 24 May 2018, 13:10
 */

#ifndef REGIONPROFILE_H
#define REGIONPROFILE_H

#include "translate/profile/RegionTable.h"

#include <string>
#include <map>

namespace archsim {
	namespace translate {
		namespace profile {
			
			/*
			 * Class representing a control flow graph for each region, for
			 * a single architecture
			 */
			class ArchRegionProfile {
			public:
				RegionTable &GetRegions() { return regions_; }
				
			private:
				RegionTable regions_;
			};
			
			/*
			 * Class managing a control flow graph for each region and arch
			 * being simulated.
			 */
			class RegionProfile {
			public:
				// For now, just use the name of the arch as it's identifier.
				// in the future we might use a UID or something
				using arch_id_t = std::string;
				
				ArchRegionProfile &Get(const arch_id_t &architecture) { return profiles_[architecture]; }
				
			private:
				std::map<arch_id_t, ArchRegionProfile> profiles_;
			};
			
		}
	}
}

#endif /* REGIONPROFILE_H */

