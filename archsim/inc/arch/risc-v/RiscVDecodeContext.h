/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
	namespace arch
	{
		namespace riscv
		{
			class RiscVDecodeContext : public gensim::DecodeContext
			{
			public:
				RiscVDecodeContext(archsim::ThreadInstance *cpu);

				/*
				 * DecodeSync is the main decode method exposed by the decode context.
				 * It should be called synchronously with the processor executing
				 * or translating instructions i.e., not for tracing.
				 */
				virtual uint32_t DecodeSync(Address address, uint32_t mode, gensim::BaseDecode &target) override;

			private:
			};
		}
	}
}

#endif /* RISCVDECODECONTEXT_H */

