/*
 * genC/ssa/printers/SSAContextPrinter.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#include "genC/ssa/printers/SSAContextPrinter.h"
#include "genC/ssa/printers/SSAActionPrinter.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::ssa::printers;

SSAContextPrinter::SSAContextPrinter(const SSAContext& context) : _context(context)
{

}

void SSAContextPrinter::Print(std::ostringstream& output_stream) const
{
	for (const auto& action : _context.Actions()) {
		SSAActionPrinter action_printer(*action.second);
		action_printer.Print(output_stream);
		output_stream << std::endl;
	}
}
