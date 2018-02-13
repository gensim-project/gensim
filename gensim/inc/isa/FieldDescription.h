/*
 * FieldDescription.h
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#ifndef INC_ISA_FIELDDESCRIPTION_H_
#define INC_ISA_FIELDDESCRIPTION_H_

#include "DiagnosticContext.h"

#include <string>

namespace gensim
{
	namespace isa
	{

		class FieldDescription
		{
		public:
			std::string field_name;
			std::string field_type;
		};

		class FieldDescriptionParser
		{
		public:
			FieldDescriptionParser(DiagnosticContext &diag);

			bool Parse(void *ptree);
			FieldDescription *Get();

		private:
			DiagnosticContext &diag;
			FieldDescription *description;
		};

	}
}



#endif /* INC_ISA_FIELDDESCRIPTION_H_ */
