/*
 * File:   RegionTable.h
 * Author: s0457958
 *
 * Created on 05 August 2014, 10:01
 */

#ifndef REGIONTABLE_H
#define	REGIONTABLE_H

#include "define.h"
#include "abi/Address.h"
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

				inline Region& Get(TranslationManager &mgr, Address phys_base)
				{
					assert(phys_base.GetPageOffset() == 0);
					profile::Region *&ptr = regions[phys_base.GetPageIndex()];
					if (ptr == nullptr) {
						ptr = InstantiateRegion(mgr, phys_base);
						dense_regions.insert(ptr);
					}
					return *ptr;
				}

				inline bool TryGet(TranslationManager &mgr, Address phys_base, Region*& region)
				{
					assert(phys_base.GetPageOffset() == 0);
					profile::Region *&ptr = regions[phys_base.GetPageIndex()];
					if (ptr == nullptr) {
						return false;
					}

					region = ptr;
					return true;
				}

				size_t Erase(Address phys_base);
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
				Region *InstantiateRegion(TranslationManager &mgr, Address phys_base);

				dense_regions_t dense_regions;
				Region *regions[profile::RegionArch::PageCount];
			};
		}
	}
}

#endif	/* REGIONTABLE_H */

