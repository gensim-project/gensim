/*
 * genC/ssa/printers/SSAActionCFGPrinter.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#include "genC/ssa/printers/SSAActionCFGPrinter.h"
#include "genC/ssa/printers/SSABlockPrinter.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ir/IR.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::printers;

SSAActionCFGPrinter::SSAActionCFGPrinter(const SSAActionBase& action) : _action(action)
{

}

void SSAActionCFGPrinter::Print(std::ostringstream& output) const
{
	const auto& ssa_form_action = dynamic_cast<const SSAFormAction &>(_action);

	output << "digraph " << ssa_form_action.GetPrototype().GetIRSignature().GetName() << "{" << std::endl;

	// First, declare basic block nodes
	for (const auto& block : ssa_form_action.Blocks) {
		output << block->GetName() << "[style=filled,fillcolor=";
		switch (block->IsFixed()) {
			case BLOCK_ALWAYS_CONST:
				output << "green";
				break;
			case BLOCK_SOMETIMES_CONST:
				output << "yellow";
				break;
			case BLOCK_NEVER_CONST:
				output << "red";
				break;
		}

		output << ",label=\"" << block->GetName() << "(";

		auto first = block->GetStatements().front();
		output << first->GetDiag().Line();

		output << ")\"];" << std::endl;
	}

	// Now, declare links between blocks
	for (const auto& block : ssa_form_action.Blocks) {
		for (const auto& successor : block->GetSuccessors()) {
			output << block->GetName() << " -> " << successor->GetName() << ";" << std::endl;
		}
	}

	output << "}" << std::endl;
}
