/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
