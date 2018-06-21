/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
/*
 * AsmDescription.h
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#ifndef INC_ISA_ASMDESCRIPTION_H_
#define INC_ISA_ASMDESCRIPTION_H_

#include "DiagnosticContext.h"

#include <string>
#include <map>
#include <vector>


namespace gensim
{
	namespace util
	{
		class expression;
	}
	namespace isa
	{

		class ISADescription;

		class AsmDescription
		{
		public:
			std::string instruction_name;
			std::string format;
			std::map<std::string, const util::expression *> constraints;
			std::vector<const util::expression *> args;

			void *parsed_format;

		};

	}
}


#endif /* INC_ISA_ASMDESCRIPTION_H_ */
