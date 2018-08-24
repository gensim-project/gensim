/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/printers/SSABlockPrinter.h"
#include "genC/ssa/printers/SSAStatementPrinter.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSASymbol.h"

using namespace gensim::genc::ssa::printers;

SSABlockPrinter::SSABlockPrinter(const SSABlock& block) : _block(block)
{

}

void SSABlockPrinter::Print(std::ostringstream& output_stream) const
{
	if (_block.GetPredecessors().size() != 0) {
		output_stream << "[";
		for (const auto& pred : _block.GetPredecessors()) {
			output_stream << pred->GetName() << ", ";
		}
		output_stream << "] -> ";
	}

	if (_block.DynamicIn.size() > 0) {
		output_stream << "{";
		for (const auto& symbol : _block.DynamicIn) {
			output_stream << symbol->GetName() << ", ";
		}
		output_stream << "}";
	}

	output_stream << "block " << _block.GetName();

	if (_block.DynamicOut.size() > 0) {
		output_stream << "{";
		for (const auto& symbol : _block.DynamicOut) {
			output_stream << symbol->GetName() << ", ";
		}
		output_stream << "}";
	}

	if (_block.GetSuccessors().size() != 0) {
		output_stream << " -> [";
		for (const auto& successor : _block.GetSuccessors()) {
			output_stream << successor->GetName() << ", ";
		}
		output_stream << "]";
	}

	switch (_block.IsFixed()) {
		case BLOCK_NEVER_CONST:
			output_stream << " never_const ";
			break;
		case BLOCK_SOMETIMES_CONST:
			output_stream << " sometimes_const ";
			break;
		case BLOCK_ALWAYS_CONST:
			output_stream << " always_const ";
			break;
		default:
			output_stream << "INVALID";
			break;
	}

	output_stream << "{" << std::endl;
	for (const auto& stmt : _block.GetStatements()) {
		SSAStatementPrinter statement_printer(*stmt);

		statement_printer.Print(output_stream);
		output_stream << std::endl;
	}
	output_stream << "}" << std::endl;
}
