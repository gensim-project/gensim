/*
 * RegionArch.h
 *
 *  Created on: 5 Aug 2014
 *      Author: harry
 */

#ifndef REGIONARCH_H_
#define REGIONARCH_H_

#include "define.h"

namespace archsim
{
	namespace translate
	{
		namespace profile
		{

			class RegionArch
			{
			public:
				RegionArch() = delete;

				static const uint32_t PageBits = 12;
				static const uint32_t PageSize = 1 << PageBits;
				static const uint32_t PageMask = PageSize - 1;
				static const uint32_t PageCount = 1 << ((32 - PageBits));
			};

		}
	}
}


#endif /* REGIONARCH_H_ */
