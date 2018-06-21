/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/printers/SSAStatementPrinter.h"
#include "genC/ssa/statement/SSAStatement.h"

using namespace gensim::genc::ssa::printers;

SSAStatementPrinter::SSAStatementPrinter(const SSAStatement& statement) : _stmt(statement)
{

}

void SSAStatementPrinter::Print(std::ostringstream& output_stream) const
{
	_stmt.PrettyPrint(output_stream);
}
