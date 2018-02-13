/*
 * File:   RegionTable.h
 * Author: s0457958
 *
 * Created on 05 August 2014, 10:01
 */

#ifndef REGIONTABLE_H
#define	REGIONTABLE_H

#include "define.h"
#include "translate/profile/RegionArch.h"

#include <unordered_set>

namespace archsim
{
	namespace translate
	{
		class TranslationManager;
		namespace profile
		{
			class Region;

			class RegionTable
			{
			public:
				typedef std::unordered_set<Region *> dense_regions_t;

				RegionTable();

				inline Region& Get(TranslationManager &mgr, phys_addr_t phys_base)
				{
					assert((phys_base & 0xfff) == 0);
					profile::Region *&ptr = regions[RegionArch::PageIndexOf(phys_base)];
					if (ptr == nullptr) {
						ptr = InstantiateRegion(mgr, phys_base);
						dense_regions.insert(ptr);
					}
					return *ptr;
				}

				inline bool TryGet(TranslationManager &mgr, phys_addr_t phys_base, Region*& region)
				{
					assert((phys_base & 0xfff) == 0);
					profile::Region *&ptr = regions[RegionArch::PageIndexOf(phys_base)];
					if (ptr == nullptr) {
						return false;
					}

					region = ptr;
					return true;
				}

				size_t Erase(phys_addr_t phys_base);
				void Clear();

				const dense_regions_t::const_iterator begin() const
				{
					return dense_regions.begin();
				}
				const dense_regions_t::const_iterator end() const
				{
					return dense_regions.end();
				}

				dense_regions_t::iterator begin()
				{
					return dense_regions.begin();
				}
				dense_regions_t::iterator end()
				{
					return dense_regions.end();
				}

			private:
				Region *InstantiateRegion(TranslationManager &mgr, phys_addr_t phys_base);

				dense_regions_t dense_regions;
				Region *regions[profile::RegionArch::PageCount];
			};
		}
	}
}

#endif	/* REGIONTABLE_H */

