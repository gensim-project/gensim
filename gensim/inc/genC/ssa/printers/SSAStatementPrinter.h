/*
 * genC/ssa/printers/SSAStatementPrinter.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "SSAPrinter.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAStatement;

			namespace printers
			{
				class SSAStatementPrinter : public SSAPrinter
				{
				public:
					SSAStatementPrinter(const SSAStatement& statement);

					void Print(std::ostringstream& output_stream) const override;

				private:
					const SSAStatement& _stmt;
				};
			}
		}
	}
}
