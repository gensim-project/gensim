/*
 * genC/ssa/printers/SSAActionCFGPrinter.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#include "genC/ssa/printers/SSAPrinter.h"
#include <iostream>

using namespace gensim::genc::ssa::printers;

std::string SSAPrinter::ToString() const
{
	std::ostringstream text;
	Print(text);

	return text.str();
}
