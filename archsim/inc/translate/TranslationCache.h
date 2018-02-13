/*
 * TranlationCache.h
 *
 *  Created on: 22 Jul 2015
 *      Author: harry
 */

#ifndef INC_TRANSLATE_TRANLATIONCACHE_H_
#define INC_TRANSLATE_TRANLATIONCACHE_H_

#include "translate/profile/RegionArch.h"
#include <bitset>

namespace archsim
{
	namespace translate
	{

		class TranslationCache
		{
		public:
			TranslationCache();

			inline void **GetPtr()
			{
				return (void**)&region_txln_cache;
			}

			inline void **GetEntry(virt_addr_t virt_addr)
			{
				uint32_t page_index = profile::RegionArch::PageIndexOf(virt_addr);
				dirty_pages.set(page_index >> cache_page_bits);
				return &region_txln_cache[page_index];
			}

			void Invalidate();
			void InvalidateEntry(virt_addr_t entry);

			void InvalidateAll();
		private:
			static const size_t cache_page_bits = 12;
			static const size_t num_cache_pages = archsim::translate::profile::RegionArch::PageCount >> cache_page_bits;
			static const size_t cache_page_size = (1 << cache_page_bits) * sizeof(void*);

			std::bitset<num_cache_pages> dirty_pages;
			void *region_txln_cache [archsim::translate::profile::RegionArch::PageCount];
		};

	}
}



#endif /* INC_TRANSLATE_TRANLATIONCACHE_H_ */
