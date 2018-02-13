/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   InstStructBuilder.h
 * Author: harry
 *
 * Created on 18 July 2017, 14:46
 */

#ifndef INSTSTRUCTBUILDER_H
#define INSTSTRUCTBUILDER_H

#include "ir/IRType.h"

namespace gensim
{
	namespace isa
	{
		class ISADescription;
	}
	namespace genc
	{
		class InstStructBuilder
		{
		public:
			IRType BuildType(const gensim::isa::ISADescription *isa) const;
		};
	}
}

#endif /* INSTSTRUCTBUILDER_H */

