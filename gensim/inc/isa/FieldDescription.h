/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
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
			FieldDescription() {}
			FieldDescription(const std::string &name, const std::string &type) : field_name(name), field_type(type) {}

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
