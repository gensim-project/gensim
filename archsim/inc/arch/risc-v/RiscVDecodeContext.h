/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   RiscVDecodeContext.h
 * Author: harry
 *
 * Created on 30 June 2017, 11:54
 */

#ifndef RISCVDECODECONTEXT_H
#define RISCVDECODECONTEXT_H

#include "gensim/gensim_decode_context.h"
#include "abi/Address.h"

namespace gensim
{
	class BaseDecode;
	class DecodeContext;
	class Processor;
}

namespace archsim
{
	class ArchDescriptor;

	namespace arch
	{
		namespace riscv
		{
			class RiscVDecodeContext : public gensim::DecodeContext
			{
			public:
				RiscVDecodeContext(const archsim::ArchDescriptor &arch) : arch_(arch) {}

				/*
				 * DecodeSync is the main decode method exposed by the decode context.
				 * It should be called synchronously with the processor executing
				 * or translating instructions i.e., not for tracing.
				 */
				virtual uint32_t DecodeSync(archsim::MemoryInterface &interface, Address address, uint32_t mode, gensim::BaseDecode *&target) override;

			private:
				const archsim::ArchDescriptor &arch_;
			};
		}
	}
}

#endif /* RISCVDECODECONTEXT_H */

