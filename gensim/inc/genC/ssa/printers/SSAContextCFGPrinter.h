/*
 * genC/ssa/printers/SSAContextCFGPrinter.h
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
			class SSAContext;

			namespace printers
			{
				class SSAContextCFGPrinter : public SSAPrinter
				{
				public:
					SSAContextCFGPrinter(const SSAContext& context);

					void Print(std::ostringstream& output_stream) const override;

				private:
					const SSAContext& _context;
				};
			}
		}
	}
}
