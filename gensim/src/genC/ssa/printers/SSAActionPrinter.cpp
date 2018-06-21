/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/printers/SSAActionPrinter.h"
#include "genC/ssa/printers/SSABlockPrinter.h"
#include "genC/ssa/SSAFormAction.h"

using namespace gensim::genc::ssa::printers;

SSAActionPrinter::SSAActionPrinter(const SSAActionBase& action) : _action(action)
{

}

void SSAActionPrinter::Print(std::ostringstream& output_stream) const
{
	const auto& ssa_form_action = dynamic_cast<const SSAFormAction &>(_action);

	output_stream << "action " << ssa_form_action.GetPrototype().GetIRSignature().GetName() << "(";

	for(const auto param : ssa_form_action.GetPrototype().GetIRSignature().GetParams()) {
		output_stream << param.GetType().PrettyPrint() << " " << param.GetName() << ", ";
	}

	output_stream << ") {" << std::endl;

	for (const auto& block : ssa_form_action.Blocks) {
		SSABlockPrinter block_printer(*block);
		block_printer.Print(output_stream);
	}

	output_stream << "}" << std::endl;
}
