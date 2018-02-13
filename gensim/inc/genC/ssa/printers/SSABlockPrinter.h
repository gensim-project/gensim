/*
 * genC/ssa/printers/SSABlockPrinter.h
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
			class SSABlock;

			namespace printers
			{
				class SSABlockPrinter : public SSAPrinter
				{
				public:
					SSABlockPrinter(const SSABlock& block);
					void Print(std::ostringstream& output_stream) const override;

				private:
					const SSABlock& _block;
				};
			}
		}
	}
}
