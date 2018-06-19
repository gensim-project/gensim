/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
