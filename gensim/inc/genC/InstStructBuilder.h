/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   InstStructBuilder.h
 * Author: harry
 *
 * Created on 18 July 2017, 14:46
 */

#ifndef INSTSTRUCTBUILDER_H
#define INSTSTRUCTBUILDER_H

#include "ir/IRType.h"
#include "isa/StructDescription.h"
#include "genC/ssa/SSATypeManager.h"

namespace gensim
{
	namespace isa
	{
		class ISADescription;
	}
	namespace genc
	{
		class StructBuilder
		{
		public:
			IRType BuildStruct(const gensim::isa::ISADescription *isa, const gensim::isa::StructDescription *struct_desc, ssa::SSATypeManager &ctx) const;

		private:
			IRType GetGenCType(const gensim::isa::ISADescription *isa, const std::string &tname, ssa::SSATypeManager &ctx) const;
		};

		class InstStructBuilder
		{
		public:
			IRType BuildType(const gensim::isa::ISADescription *isa, ssa::SSATypeManager &ctx) const;
		};
	}
}

#endif /* INSTSTRUCTBUILDER_H */

