/*
 * genC/ssa/printers/SSAStatementPrinter.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
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
