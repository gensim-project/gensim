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

		class AsmDescriptionParser
		{
		public:
			AsmDescriptionParser(DiagnosticContext &diag, std::string filename);

			bool Parse(void *tree, const ISADescription &format);

			AsmDescription *Get();
		private:
			AsmDescription *description;
			DiagnosticContext &diag;
			std::string filename;
		};

	}
}


#endif /* INC_ISA_ASMDESCRIPTION_H_ */
