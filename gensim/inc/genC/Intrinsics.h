/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * genC/Intrinsics.h
 *
 * GenSim
 * Copyright (C) University of Edinburgh.  All Rights Reserved.
 *
 * Harry Wagstaff <hwagstaf@inf.ed.ac.uk>
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ir/IRType.h"

#include <string>

namespace gensim
{
	namespace genc
	{
		class IntrinsicDefinition
		{
		public:

		};

		class InternalIntrinsicDefinition : public IntrinsicDefinition
		{
		public:
			InternalIntrinsicDefinition(const std::string& name);
		};

		class ExternalIntrinsicDefinition : public IntrinsicDefinition
		{
		public:
			ExternalIntrinsicDefinition(const IRType& ret_type, const std::string& name, ...);
		};
	}
}
