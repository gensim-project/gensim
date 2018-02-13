/*
 * genC/ssa/printers/SSAActionPrinter.h
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
			class SSAActionBase;

			namespace printers
			{
				class SSAActionPrinter : public SSAPrinter
				{
				public:
					SSAActionPrinter(const SSAActionBase& action);
					void Print(std::ostringstream& output_stream) const override;

				private:
					const SSAActionBase& _action;
				};
			}
		}
	}
}
