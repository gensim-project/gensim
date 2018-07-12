/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SSATypeFormatter.h
 * Author: harry
 *
 * Created on 11 July 2018, 16:04
 */

#ifndef SSATYPEFORMATTER_H
#define SSATYPEFORMATTER_H

#include "genC/ir/IRType.h"

#include <string>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSATypeFormatter
			{
			public:
				void SetStructPrefix(const std::string &sp)
				{
					struct_prefix_ = sp;
				}

				std::string FormatType(const IRType &type) const;

			private:
				std::string struct_prefix_;
			};
		}
	}
}

#endif /* SSATYPEFORMATTER_H */

