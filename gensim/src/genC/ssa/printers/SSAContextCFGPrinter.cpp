/*
 * genC/ssa/printers/SSAContextCFGPrinter.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#include "genC/ssa/printers/SSAContextCFGPrinter.h"
#include "genC/ssa/printers/SSAActionCFGPrinter.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::ssa::printers;

SSAContextCFGPrinter::SSAContextCFGPrinter(const SSAContext& context) : _context(context)
{

}

void SSAContextCFGPrinter::Print(std::ostringstream& output_stream) const
{
	for (const auto& action : _context.Actions()) {
		SSAActionCFGPrinter action_cfg_printer(*action.second);
		action_cfg_printer.Print(output_stream);

		output_stream << std::endl;
	}
}
