/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSATypeFormatter.h"

using namespace gensim::genc::ssa;

std::string SSATypeFormatter::FormatType(const IRType& type) const
{
	if(type.IsStruct()) {
		return struct_prefix_ + type.BaseType.StructType->Name;
	} else {
		return type.GetCType();
	}
}