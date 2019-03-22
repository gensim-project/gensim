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

namespace gensim
{
	namespace genc
	{
		enum class IntrinsicID {
			UNKNOWN,

#define Intrinsic(name, id, ...) id,
#include "Intrinsics.def"
#undef Intrinsic

			END
		};
	}
}
