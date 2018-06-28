/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
