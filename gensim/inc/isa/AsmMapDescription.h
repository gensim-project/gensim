/*
 * AsmMapDescription.h
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#ifndef INC_ISA_ASMMAPDESCRIPTION_H_
#define INC_ISA_ASMMAPDESCRIPTION_H_

#include "Util.h"

#include <algorithm>
#include <map>
#include <string>

namespace gensim
{
	namespace isa
	{

		class AsmMapDescription
		{
		public:
			std::string Name;
			std::map<int, std::string> mapping;

			AsmMapDescription() {};
			AsmMapDescription(void *node);

			int GetMin() const
			{
				return std::min_element(mapping.begin(), mapping.end(), util::MapFirstComparison<int, std::string>())->first;
			}

			int GetMax() const
			{
				return std::max_element(mapping.begin(), mapping.end(), util::MapFirstComparison<int, std::string>())->first;
			}
		};

	}
}


#endif /* INC_ISA_ASMMAPDESCRIPTION_H_ */
