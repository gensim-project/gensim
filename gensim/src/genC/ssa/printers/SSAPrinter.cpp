/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/printers/SSAPrinter.h"
#include <iostream>

using namespace gensim::genc::ssa::printers;

std::string SSAPrinter::ToString() const
{
	std::ostringstream text;
	Print(text);

	return text.str();
}
