/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
				class SSAContextPrinter : public SSAPrinter
				{
				public:
					SSAContextPrinter(const SSAContext& context);

					void Print(std::ostringstream& output_stream) const override;

				private:
					const SSAContext& _context;
				};
			}
		}
	}
}
