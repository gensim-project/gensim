/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
