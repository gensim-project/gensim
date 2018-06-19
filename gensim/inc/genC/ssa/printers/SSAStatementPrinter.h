/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
