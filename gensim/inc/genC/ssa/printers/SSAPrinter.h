/*
 * genC/ssa/printers/SSAPrinter.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include <sstream>
#include <string>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			namespace printers
			{
				class SSAPrinter
				{
				public:
					virtual void Print(std::ostringstream& output_stream) const = 0;
					std::string ToString() const;
				};
			}
		}
	}
}
